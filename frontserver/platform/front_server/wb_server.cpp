#include <boost/shared_ptr.hpp>
#include <functional>
#include "wb_server.h"
#include "front_server.h"
#include "pandora/util/json.hpp"
#include "../util/tools.h"
#include "../config/config.h"
#include "../log/log.h"
#include <sstream>

using namespace std::placeholders;

WBServer::WBServer()
{
    server_port_ = CONFIG->get_ws_port();

    init_websocket_options();

    init_websocket_behavior();

    init_websocket_ssl_server();

    init_websocket_server();
}

WBServer::~WBServer()
{
    if (!listen_thread_ && listen_thread_->joinable())
    {
        listen_thread_->join();
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


void WBServer::on_open(uWS::WebSocket<false, true> * ws)
{
    wss_con_set_.emplace(ws);

    // front_server_->request_all_symbol();

    ws->send(front_server_->get_symbols_str(), uWS::OpCode::TEXT);
}

// 处理各种请求
void WBServer::on_message(uWS::WebSocket<false, true> * ws, std::string_view msg, uWS::OpCode code)
{
    cout << "Req Msg: " << msg << endl;
    ws->send("This is Cplus", code);
    string trans_msg(msg.data(), msg.size());

    cout << "trans_msg: " << trans_msg << endl;

    process_sub_info(trans_msg, ws);
}

void WBServer::on_ping(uWS::WebSocket<false, true> * ws)
{

}

void WBServer::on_pong(uWS::WebSocket<false, true> * ws)
{

}

void WBServer::on_close(uWS::WebSocket<false, true> * ws)
{
    cout << "one connection closed " << endl;
    if (wss_con_set_.find(ws) != wss_con_set_.end())
    {
        wss_con_set_.erase(ws);
    }    
}

void WBServer::listen()
{
    cout << "WServer Start Listen: " << server_port_ << endl;

    uWS::App().ws<PerSocketData>("/*", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "WS Listening on port " << server_port_ << std::endl;
        }
    }).run();
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
    for (uWS::WebSocket<false, true> * ws: wss_con_set_)
    {
        ws->send(msg, uWS::OpCode::TEXT);
    }
}

void WBServer::process_sub_info(string ori_msg, uWS::WebSocket<false, true> * ws)
{
    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];
            for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
            {
                string cur_symbol = *it;
                ws_sub_map_[cur_symbol].insert(ws);
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

void WBServer::broadcast_enhanced_data(EnhancedDepthData& en_depth_data)
{
    string update_symbol = en_depth_data.depth_data_.symbol;

    if (ws_sub_map_.find(update_symbol) != ws_sub_map_.end())
    {
        string send_str = en_depth_data.get_json_str();
        cout << "send_str: " << send_str << endl;

        for (uWS::WebSocket<false, true>* ws:ws_sub_map_[update_symbol])
        {
            ws->send(send_str, uWS::OpCode::TEXT);
        }
    }
}