#include <boost/shared_ptr.hpp>
#include <functional>
#include <sstream>

#include "pandora/util/json.hpp"
#include "pandora/util/time_util.h"

#include "wb_server.h"
#include "front_server.h"

#include "../util/tools.h"
#include "../config/config.h"
#include "../log/log.h"

#include "../front_server_declare.h"
#include "../ErrorDefine.hpp"


using namespace std::placeholders;

WBServer::WBServer()
{
    server_port_ = CONFIG->get_ws_port();

    init_websocket_options();

    init_websocket_behavior();

    init_websocket_ssl_server();

    init_websocket_server();

    start_heartbeat();
}

WBServer::~WBServer()
{
    if (!listen_thread_ && listen_thread_->joinable())
    {
        listen_thread_->join();
    }

    if (!heartbeat_thread_ && heartbeat_thread_->joinable())
    {
        heartbeat_thread_->join();
    }    
}

void WBServer::init_websocket_options()
{

}

void WBServer::init_websocket_behavior()
{
    websocket_behavior_.open = std::bind(&WBServer::on_open, this, std::placeholders::_1);
    websocket_behavior_.message = std::bind(&WBServer::on_message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    websocket_behavior_.ping = std::bind(&WBServer::on_ping, this, std::placeholders::_1);
    websocket_behavior_.pong = std::bind(&WBServer::on_pong, this, std::placeholders::_1);
    websocket_behavior_.close = std::bind(&WBServer::on_close, this, std::placeholders::_1);
}

void WBServer::init_websocket_ssl_server()
{
}

void WBServer::init_websocket_server()
{
    // wss_server_(std::move(uWS::App().ws<PerSocketData>("/*", std::move(websocket_behavior_))));
}

void WBServer::on_open(WebsocketClass * ws)
{
    WebsocketClassThreadSafePtr ws_safe = boost::make_shared<WebsocketClassThreadSafe>(ws);

    ws_safe->set_alive(true);

    wss_con_set_.insert(ws_safe);

    string symbols_str = front_server_->get_symbols_str();

    cout << "all symbols str: " << symbols_str << endl;

    ws->send(symbols_str, uWS::OpCode::TEXT);
}

// 处理各种请求
void WBServer::on_message(WebsocketClass * ws, std::string_view msg, uWS::OpCode code)
{
    // cout << utrade::pandora::NanoTimeStr() << " Req Msg: " << msg << endl;
    string trans_msg(msg.data(), msg.size());

    // cout << "trans_msg: " << trans_msg << endl;

    // LOG_INFO(trans_msg);

    process_on_message(trans_msg, ws);
}

void WBServer::on_ping(WebsocketClass * ws)
{

}

void WBServer::on_pong(WebsocketClass * ws)
{

}

void WBServer::on_close(WebsocketClass * ws)
{
    cout << "one connection closed " << endl;

    WebsocketClassThreadSafePtr thread_safe_ws = boost::make_shared<WebsocketClassThreadSafe>(ws);
    clean_client(thread_safe_ws);
}

void WBServer::listen()
{
    cout << "WServer Start Listen: " << server_port_ << endl;

    uWS::App().ws<WSData>("/*", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "WS Listening on port " << server_port_ << std::endl;
        }
    }).run();

    cout << "WServer Listen End!" << endl;
}

void WBServer::set_front_server(FrontServer* front_server)
{
    front_server_ = front_server;
}

void WBServer::launch()
{
    listen_thread_ = boost::make_shared<std::thread>(&WBServer::listen, this);
}

void WBServer::release()
{

}

void WBServer::broadcast(string msg)
{
    cout << "broadcast: " << msg << endl;
    for (auto iter:wss_con_set_)
    {
        iter->send(msg);
    }
}

void WBServer::process_on_message(string ori_msg, WebsocketClass * ws)
{
    try
    {
        nlohmann::json js = nlohmann::json::parse(ori_msg);

        if (js["type"].is_null())
        {
            ws->send(get_error_send_rsp_string("Lost type Item!"), uWS::OpCode::TEXT);
        }
        else
        {
            if (js["type"].get<string>() == "sub_symbol")
            {
                process_sub_info(ori_msg, ws);
            }

            if (js["type"].get<string>() == HEARTBEAT)
            {
                process_heartbeat(ws);
            }

            if (js["type"].get<string>() == KLINE_UPDATE)
            {
                process_kline_data(ori_msg, ws);
            }            
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }        
}

void WBServer::process_kline_data(string ori_msg, WebsocketClass* ws)
{
    // return;  

    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        int error_id = 0;
        string err_msg ="";
        string symbol;
        type_tick start_time;
        type_tick end_time;
        type_tick frequency;

        if (js["symbol"].is_null())
        {
            err_msg += "Lost Symbol Item \n";
            error_id = -1;
        }
        else
        {
            symbol = js["symbol"].get<string>();
        }
        

        if (js["start_time"].is_null())
        {
            err_msg += "Lost start_time Item \n";
            error_id = -1;
        }
        else
        {
            start_time = std::stoul(js["start_time"].get<string>());
        }
        

        if (js["end_time"].is_null())
        {
            err_msg += "Lost end_time Item \n";
            error_id = -1;
        }
        else
        {
            end_time = std::stoul(js["end_time"].get<string>());
        }
        

        if (js["frequency"].is_null())
        {
            err_msg += "Lost frequency Item \n";
            error_id = -1;
        }        
        else
        {
            frequency = std::stoi(js["frequency"].get<string>());
            error_id = -1;
        }
                        

        cout<< "symbol: " << symbol << " \n" 
            << "start_time: " << start_time << " \n"
            << "end_time: " << end_time << " \n"
            << "frequency: " << frequency << " \n"
            << endl;

        ReqKLineData req_kline_data(symbol, start_time, end_time, frequency, nullptr, ws);

        if (error_id != 0)
        {
            front_server_->request_kline_data(req_kline_data);                     
        }
        else
        {
            ws->send(get_error_send_rsp_string(err_msg), uWS::OpCode::TEXT);
        }

    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;

        stream_msg << "WBServer::process_kline_data " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    
}

void WBServer::process_sub_info(string ori_msg, WebsocketClass * ws)
{
    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];

            WebsocketClassThreadSafePtr tmp_ws = boost::make_shared<WebsocketClassThreadSafe>(ws);

            auto iter = wss_con_set_.find(tmp_ws);

            if (iter != wss_con_set_.end())
            {
                (*iter)->clear_sub_symbol_list();

                for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
                {
                    string cur_symbol = *it;
                    
                    (*iter)->add_sub_symbol(cur_symbol);                    
                }                
            }
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;

        stream_msg << "WBServer::process_sub_info " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
}

void WBServer::process_heartbeat(WebsocketClass* ws)
{
    try
    {
        WebsocketClassThreadSafePtr tmp_ws = boost::make_shared<WebsocketClassThreadSafe>(ws);

        auto iter = wss_con_set_.find(tmp_ws);

        if (iter != wss_con_set_.end())
        {
            (*iter)->set_alive(true);
        }        
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "WBServer::process_heartbeat " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }    
}

void WBServer::broadcast_enhanced_data(string symbol, string data_str)
{
    for (auto& iter:wss_con_set_)
    {        
        if (iter->is_symbol_subed((symbol)))
        {
            iter->send(data_str);
        }        
    }
}

void WBServer::clean_client(WebsocketClassThreadSafePtr ws)
{
    if (wss_con_set_.find(ws) != wss_con_set_.end())
    {
        wss_con_set_.erase(ws);
    }
}

void WBServer::start_heartbeat()
{
    heartbeat_thread_ = boost::make_shared<std::thread>(&WBServer::heartbeat_run, this);
}

void WBServer::heartbeat_run()
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(heartbeat_seconds_));

        check_heartbeat();
    }
}

void WBServer::check_heartbeat()
{
    std::set<WebsocketClassThreadSafePtr, LessWebsocketClassThreadSafePtr> dead_ws_set;

    for (auto ws:wss_con_set_)
    {        
        if (!ws->is_alive())
        {
            dead_ws_set.emplace(ws);            
        }        
    }

    for (auto ws:dead_ws_set)
    {
        clean_client(ws);
        ws->end();
    }

    string heartbeat_str = get_heartbeat_str();

    // cout << heartbeat_str << endl;

    for (auto ws:wss_con_set_)
    {
        ws->set_alive(false);
        ws->send(heartbeat_str);
    }
}

string WBServer::get_error_send_rsp_string(string err_msg)
{
    nlohmann::json json_obj;
    json_obj["type"] = "error";
    json_obj["error_id"] = LOST_TYPE_ITEM;
    json_obj["error_msg"] = err_msg;
    return json_obj.dump();
}

string WBServer::get_heartbeat_str()
{
    nlohmann::json json_obj;
    json_obj["type"] = "heartbeat";
    json_obj["time"] = utrade::pandora::NanoTimeStr();
    return json_obj.dump();
}