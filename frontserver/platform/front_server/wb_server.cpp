#include <boost/shared_ptr.hpp>
#include <functional>
#include <sstream>

#include "pandora/util/json.hpp"
#include "pandora/util/time_util.h"

#include "wb_server.h"
#include "front_server.h"

#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../ErrorDefine.hpp"
#include "../util/id.hpp"
#include "../config/config.h"
#include "../log/log.h"

#include "../front_server_declare.h"


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

void WBServer::set_front_server(FrontServer* front_server)
{
    front_server_ = front_server;
}

void WBServer::on_open(WebsocketClass * ws)
{
    process_on_open(ws);
}

// 处理各种请求
void WBServer::on_message(WebsocketClass * ws, std::string_view msg, uWS::OpCode code)
{
    try
    {
        string trans_msg(msg.data(), msg.size());
        process_on_message(trans_msg, ws);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception! ");
    }

}

void WBServer::on_ping(WebsocketClass * ws)
{

}

void WBServer::on_pong(WebsocketClass * ws)
{

}

void WBServer::on_close(WebsocketClass * ws)
{
    try
    {
        std::stringstream s_s;
        s_s << "WBServer::on_close "<< ws;
        LOG_CLIENT_REQUEST(s_s.str());
        clean_ws(ws);
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_close: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_close: unknown exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void WBServer::listen()
{
    try
    {
        uWS::App().ws<WSData>("/*", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
            if (token) {
                LOG_INFO("WServer Start Listen: " + std::to_string(server_port_));
            }
        }).run();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception! ");
    }
    

}

void WBServer::launch()
{
    try
    {
        LOG_INFO("WServer launch");
        listen_thread_ = std::make_shared<std::thread>(&WBServer::listen, this);

        if (!listen_thread_)
        {
            LOG_ERROR("Create Listen Thread Failed!");
            exit(0);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void WBServer::start_heartbeat()
{
    try
    {
        LOG_INFO("WServer Start Heartbeat");
        heartbeat_thread_ = std::make_shared<std::thread>(&WBServer::heartbeat_run, this);

        if (!heartbeat_thread_)
        {
            LOG_ERROR("Create Heartbeat Thread Failed!");
            exit(0);
        }        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }
}

void WBServer::release()
{

}

void WBServer::process_on_open(WebsocketClass * ws)
{
    try
    {
        std::stringstream s_s;
        s_s << "WBServer::process_on_open ws: " << ws;
        LOG_CLIENT_REQUEST(s_s.str());

        ID_TYPE socket_id = store_ws(ws);

        request_symbol_list(socket_id);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }
}

void WBServer::process_on_message(string ori_msg, WebsocketClass * ws)
{
    try
    {
        std::stringstream s_s;
        s_s << "WBServer::process_on_message " << ws << ": " << ori_msg;
        // LOG_CLIENT_REQUEST(s_s.str());

        ID_TYPE socket_id = check_ws(ws);

        if (socket_id && wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            WebsocketClassThreadSafePtr ws_safe = wss_con_map_[socket_id];

            ws_safe->set_recv_heartbeat(utrade::pandora::NanoTime());

            nlohmann::json js = nlohmann::json::parse(ori_msg);

            if (js["type"].is_null())
            {
                ws->send(get_error_send_rsp_string("Lost type Item!"), uWS::OpCode::TEXT);
            }
            else
            {
                if (js["type"].get<string>() == "sub_symbol")
                {
                    process_depth_req(ori_msg, socket_id, ws_safe);

                    process_trade_req(ori_msg, socket_id, ws_safe);
                }

                if (js["type"].get<string>() == HEARTBEAT)
                {
                    process_heartbeat(socket_id, ws_safe);
                }

                if (js["type"].get<string>() == KLINE_UPDATE)
                {
                    process_kline_req(ori_msg, socket_id, ws_safe);
                }     

                if (js["type"].get<string>() == TRADE)
                {
                    process_trade_req(ori_msg, socket_id, ws_safe);
                }   
            }
        }
        else
        {
            std::stringstream s_s;
            s_s << "WBServer::process_on_message ws: " << ws << " id is invalid ";
            LOG_CLIENT_REQUEST(s_s.str());
        }
 
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }      
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }    
}

void WBServer::process_depth_req(string ori_msg, ID_TYPE socket_id, WebsocketClassThreadSafePtr ws)
{
    try
    {
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];

            for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
            {
                string cur_symbol = *it;

                LOG_CLIENT_REQUEST("socket_id: " + std::to_string(socket_id) + " req_depth: " + cur_symbol);
                    
                PackagePtr request_depth_package = CreatePackage<ReqRiskCtrledDepthData>(cur_symbol, ws, socket_id);
                if (request_depth_package)
                {
                    request_depth_package->prepare_request(UT_FID_ReqRiskCtrledDepthData, ID_MANAGER->get_id());

                    auto p_req_depth = GetField<ReqRiskCtrledDepthData>(request_depth_package);
                    LOG_CLIENT_REQUEST(p_req_depth->str());
                    
                    front_server_->deliver_request(request_depth_package);

                    front_server_->add_sub_depth(p_req_depth);
                }
                else
                {
                    LOG_ERROR("GetReqRiskCtrledDepthDataPackage Failed");
                }                
            }     
        }
        else
        {
            string err_msg = "ReqSymbolList Json Need Symbol Item";
            LOG_ERROR(err_msg);

            if (wss_con_map_.find(socket_id) != wss_con_map_.end())
            {
                wss_con_map_[socket_id]->send(get_error_send_rsp_string(err_msg));
            }
            else
            {
                stringstream s_obj;
                s_obj << "Send error msg websocket socket_id " << socket_id << " is invalid \n";
                LOG_ERROR(s_obj.str());
            }            
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }    
}

void WBServer::process_kline_req(string ori_msg, ID_TYPE socket_id, WebsocketClassThreadSafePtr ws)
{
    try
    {
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        int error_id = 0;
        string err_msg ="";
        string symbol;
        type_tick start_time = 0;
        type_tick end_time = 0;
        type_tick frequency;
        int data_count = -1;

        if (js["symbol"].is_null())
        {
            err_msg += "Lost Symbol Item \n";
            error_id = -1;
        }
        else
        {
            symbol = js["symbol"].get<string>();
        }
        

        if (!js["start_time"].is_null() && !js["end_time"].is_null())
        {
            start_time = std::stoul(js["start_time"].get<string>());
            end_time = std::stoul(js["end_time"].get<string>());
        }
        else if (!js["data_count"].is_null())
        {
            data_count = std::stoul(js["data_count"].get<string>());
        }
        else
        {
            err_msg += "K line data needs start_end time or data_count! \n";
            error_id = -1;
        }

        if (js["frequency"].is_null())
        {
            err_msg += "Lost frequency Item \n";
            error_id = -1;
        }        
        else
        {
            frequency = std::stoi(js["frequency"].get<string>());
        }

        stringstream info_obj;
        info_obj << "socket_id: " << socket_id << "\n"
                << "req_kline: " << "\n"
                << "symbol: " << symbol << " \n" 
                << "start_time: " << start_time << " \n"
                << "end_time: " << end_time << " \n"
                << "frequency: " << frequency << " \n"
                << "data_count: " << data_count << "\n";

        LOG_CLIENT_REQUEST(info_obj.str());

        { 
            if (error_id == 0)
            {
                COMM_TYPE socket_type = COMM_TYPE::WEBSOCKET;
                PackagePtr package = CreatePackage<ReqKLineData>(symbol, start_time, end_time, data_count, frequency, 
                                                                 ws, socket_id);
                
                if (package)
                {
                    package->prepare_request(UT_FID_ReqKLineData, package->PackageID());

                    auto req_kline_data = GetField<ReqKLineData>(package);

                    if (!req_kline_data)
                    {
                        LOG_ERROR("req_kline_data empty!");
                        return;
                    }

                    front_server_->add_sub_kline(req_kline_data);
                    front_server_->deliver_request(package);
                }
                else
                {
                    LOG_ERROR("WBServer::process_kline_req CreatePackage Failed!");
                }            
            }
            else
            {
                LOG_ERROR(err_msg);

                if (wss_con_map_.find(socket_id) != wss_con_map_.end())
                {
                    wss_con_map_[socket_id]->send(get_error_send_rsp_string(err_msg));
                }
                else
                {
                    stringstream s_obj;
                    s_obj << "WBServer::process_kline_req websocket socket_id " << socket_id << " is invalid \n";
                    LOG_ERROR(s_obj.str());
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }    
}

void WBServer::process_trade_req(string ori_msg, ID_TYPE socket_id, WebsocketClassThreadSafePtr ws)
{
    try
    {
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];
            for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
            {
                string symbol = *it;
                LOG_CLIENT_REQUEST("socket_id: " + std::to_string(socket_id) + " req_trade: " + symbol);

                PackagePtr package = CreatePackage<ReqTrade>(symbol, false, ws, socket_id);

                if (package)
                {
                    auto p_req_trade = GetField<ReqTrade>(package);

                    front_server_->add_sub_trade(p_req_trade);

                    package->prepare_request(UT_FID_ReqTrade, ID_MANAGER->get_id());
                    front_server_->deliver_request(package);
                }
                else
                {
                    stringstream stream_msg;
                    stream_msg << "WBServer::process_trade_req Create Package Failed " << symbol << " " << socket_id << "\n";                
                    LOG_ERROR(stream_msg.str());
                }          
            }     
        }       
        else
        {
            string err_msg = "ReqTrade Json Need Symbol Item";
            LOG_ERROR(err_msg);

            if (wss_con_map_.find(socket_id) != wss_con_map_.end())
            {
                wss_con_map_[socket_id]->send(get_error_send_rsp_string(err_msg));
            }
            else
            {
                stringstream s_obj;
                s_obj << "WBServer::process_trade_req send error msg websocket socket_id " << socket_id << " is invalid \n";
                LOG_ERROR(s_obj.str());
            }                        
        }         
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }  
    
}

void WBServer::process_heartbeat(ID_TYPE socket_id, WebsocketClassThreadSafePtr ws)
{
    try
    {
        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            wss_con_map_[socket_id]->set_recv_heartbeat(utrade::pandora::NanoTime());
        }        
        else
        {
            LOG_WARN("process_heartbeat socket_id: " + std::to_string(socket_id) + " is invalid ");
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] WBServer::process_heartbeat " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::process_heartbeat: unknown exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void WBServer::request_symbol_list(ID_TYPE socket_id)
{
    try
    {
        if (socket_id && wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            WebsocketClassThreadSafePtr ws_safe = wss_con_map_[socket_id];

            // PackagePtr package =  GetReqSymbolListDataPackage(socket_id, COMM_TYPE::WEBSOCKET, ID_MANAGER->get_id());

            PackagePtr package = CreatePackage<ReqSymbolListData>(ws_safe); 
            if (package)
            {
                ReqSymbolListDataPtr pReqSymbolListData = GetField<ReqSymbolListData>(package);

                if (pReqSymbolListData)
                {
                    LOG_CLIENT_REQUEST(pReqSymbolListData->str());

                    package->prepare_request(UT_FID_ReqSymbolListData, ID_MANAGER->get_id());

                    front_server_->add_sub_symbol_list(pReqSymbolListData);
            
                    front_server_->deliver_request(package);  
                }              
            }
            else
            {
                LOG_ERROR("CreatePackage<ReqSymbolListData> Failed!");
            }
        }
        else
        {
            LOG_ERROR("SocketID " + std::to_string(socket_id) + " is invalid");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }
}

bool WBServer::send_data(ID_TYPE socket_id, string msg)
{
    try
    {
        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            wss_con_map_[socket_id]->send(msg);
            return true;
        }
        else
        {
            stringstream s_obj;
            s_obj << "websocket socket_id " << socket_id << " is invalid \n";
            LOG_ERROR(s_obj.str());
            return false;
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }    

    return false;
}

void WBServer::heartbeat_run()
{
    try
    {
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(CONFIG->get_heartbeat_secs()-2));

            check_heartbeat();
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::heartbeat_run: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::heartbeat_run: unknown exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void WBServer::check_heartbeat()
{
    try
    {
        {
            std::vector<ID_TYPE> ws_vec;    

            std::lock_guard<std::mutex> lk(wss_con_mutex_);
            for (auto iter:wss_con_map_)
            {
                if (!iter.second->is_alive())
                {
                    ws_vec.push_back(iter.first);

                    LOG_WARN("check_heartbeat " + iter.second->get_ws_str() + " lost heartbeat");
                }
                else
                {
                    // cout <<"\n[H] " << utrade::pandora::NanoTimeStr() << " id: " << iter.first << " check heartbeat Successfully!" << endl;                    
                }
            }
                
            for (auto socket_id:ws_vec)
            {
                close_ws(socket_id);
            }     
        }   

        string heartbeat_str = get_heartbeat_str();    
        for (auto iter:wss_con_map_)
        {
            iter.second->set_new_business_request(false);
            // iter.second->set_send_heartbeat(utrade::pandora::NanoTime());

            iter.second->send(heartbeat_str);
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_heartbeat: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_heartbeat: unknown exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }

}

ID_TYPE WBServer::store_ws(WebsocketClass * ws)
{
    try
    {                
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();

        
        LOG_INFO("store original socket_id: " + std::to_string(socket_id));

        if (!socket_id)
        {
            socket_id = ID_MANAGER->get_id();
            data_ptr->set_id(socket_id);
        }

        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) == wss_con_map_.end())
        {
            WebsocketClassThreadSafePtr ws_safe = boost::make_shared<WebsocketClassThreadSafe>(ws, socket_id);
            ws_safe->set_recv_heartbeat(utrade::pandora::NanoTime());

            ws_safe->set_new_business_request(true);
            wss_con_map_[socket_id] = ws_safe;

            LOG_INFO("store new ws: " + ws_safe->get_ws_str());
            LOG_CLIENT_REQUEST("store new ws: " + ws_safe->get_ws_str());
        }
        return socket_id;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }

    return 0;
}

ID_TYPE WBServer::check_ws(WebsocketClass * ws)
{
    try
    {
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();
        if (socket_id)
        {
            if (wss_con_map_.find(socket_id) == wss_con_map_.end())
            {
                stringstream s_obj;
                s_obj << "ws " << ws << " id: " << socket_id << " has already been closed!" << "\n";
                LOG_WARN(s_obj.str());

                data_ptr->set_id(0);
            }
            else
            {
                wss_con_map_[socket_id]->set_new_business_request(true);
                wss_con_map_[socket_id]->set_recv_heartbeat(utrade::pandora::NanoTime());
                LOG_TRACE(wss_con_map_[socket_id]->get_ws_str() + " heartbeat_time: " + wss_con_map_[socket_id]->get_recv_heart_beate_time_str());
            }
        }
        else
        {
            std::stringstream s_s;
            s_s << "ws " << ws << " is invalid now!";
            LOG_WARN(s_s.str());
        }

        return data_ptr->get_id();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }

    return 0;
}

ID_TYPE WBServer::clean_ws(WebsocketClass* ws)
{
    try
    {
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();
        stringstream s_obj;

        if (socket_id)
        {
            data_ptr->set_id(0);
        
            std::lock_guard<std::mutex> lk(wss_con_mutex_);
            if (wss_con_map_.find(socket_id) != wss_con_map_.end())
            {
                wss_con_map_[socket_id]->set_new_business_request(false);
                wss_con_map_[socket_id]->set_recv_heartbeat(0);

                wss_con_map_.erase(socket_id);

                s_obj << "Clean Socket: " << ws  << ", id: " << socket_id << " successfully!";
                LOG_CLIENT_REQUEST(s_obj.str());
            }
            else
            {
                s_obj << "Socket: " << ws << ", id: " << socket_id << " has already been closed!";
                LOG_CLIENT_REQUEST(s_obj.str());
            }
            return socket_id;
        
        }
        else
        {
            s_obj << "Socket: " << ws << " has already been closed!";
            LOG_WARN(s_obj.str());
        }

        return 0;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }

    return 0;
}

void WBServer::close_ws(ID_TYPE socket_id)
{
    try
    {
        if (wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            close_ws(wss_con_map_[socket_id]);
        }
        else
        {
            LOG_WARN("Socket: " + wss_con_map_[socket_id]->get_ws_str()  + ", id: " + std::to_string(socket_id) + " has already been closed!");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void WBServer::close_ws(WebsocketClassThreadSafePtr ws_safe)
{
    try
    {
        if (wss_con_map_.find(ws_safe->get_id()) == wss_con_map_.end())
        {
            LOG_WARN("Socket: " + ws_safe->get_ws_str()  + ", id: " + std::to_string(ws_safe->get_id()) + " has already been closed!");
        }
        else
        {
            WebsocketClass * ws = ws_safe->get_ws();
            WSData* data_ptr = (WSData*)(ws->getUserData());
            data_ptr->set_id(0);

            LOG_WARN("Close: " + ws_safe->get_ws_str());
            ws->close();

            ws_safe->set_new_business_request(false);
            ws_safe->set_recv_heartbeat(0);            
            wss_con_map_.erase(ws_safe->get_id());
        }              
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

ID_TYPE WBServer::get_socket_id(WebsocketClass * ws)
{
    try
    {
        WSData* data_ptr = (WSData*)(ws->getUserData());
        return data_ptr->get_id();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return 0;
}