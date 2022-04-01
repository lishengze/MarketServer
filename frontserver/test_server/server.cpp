#include <boost/shared_ptr.hpp>
#include <functional>
#include "server.h"

using namespace std::placeholders;

Server::Server()
{
    init_websocket_options();

    init_websocket_behavior();

    init_websocket_ssl_server();

    init_websocket_server();
}

Server::~Server()
{

}

void Server::init_websocket_options()
{

}

void Server::init_websocket_behavior()
{
    websocket_behavior_.open = std::bind(&Server::on_open, this, _1);
    websocket_behavior_.message = std::bind(&Server::on_message, this, _1, _2, _3);
    websocket_behavior_.ping = std::bind(&Server::on_ping, this, _1);
    websocket_behavior_.pong = std::bind(&Server::on_pong, this, _1);
    websocket_behavior_.close = std::bind(&Server::on_close, this, _1);

}

void Server::init_websocket_ssl_server()
{
}

void Server::init_websocket_server()
{
    // wss_server_(std::move(uWS::App().ws<PerSocketData>("/*", std::move(websocket_behavior_))));
}


void Server::on_open(uWS::WebSocket<false, true> * ws)
{
    wss_con_set_.emplace(ws);
}

void Server::on_message(uWS::WebSocket<false, true> * ws, std::string_view msg, uWS::OpCode code)
{
    cout << "Req Msg: " << msg << endl;
    ws->send("This is Cplus", code);
}

void Server::on_ping(uWS::WebSocket<false, true> * ws)
{

}

void Server::on_pong(uWS::WebSocket<false, true> * ws)
{

}

void Server::on_close(uWS::WebSocket<false, true> * ws)
{
    if (wss_con_set_.find(ws) != wss_con_set_.end())
    {
        wss_con_set_.erase(ws);
    }    
}


void Server::start()
{
    cout << "Start Listen: " << server_port_ << endl;

    uWS::App().ws<PerSocketData>("/trading/market", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "Listening on port " << server_port_ << std::endl;
        }
    }).run();
}