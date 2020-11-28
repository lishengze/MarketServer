#include <boost/shared_ptr.hpp>
#include <functional>
#include "wb_server.h"

using namespace std::placeholders;

WBServer::WBServer()
{
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
}

// 处理各种请求
void WBServer::on_message(uWS::WebSocket<false, true> * ws, std::string_view msg, uWS::OpCode code)
{
    cout << "Req Msg: " << msg << endl;
    ws->send("This is Cplus", code);
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
    cout << "Start Listen: " << server_port_ << endl;

    uWS::App().ws<PerSocketData>("/*", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "Listening on port " << server_port_ << std::endl;
        }
    }).run();
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