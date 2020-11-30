#pragma once

#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <string_view>

#include "App.h"
#include "libusockets.h"

#include <vector>
#include <set>

#include "../front_server_declare.h"

using std::string;
using std::cout;
using std::endl;
using std::vector;

class FrontServer;

struct PerSocketData {
    /* Fill with user data */
};

class WBServer
{
    public:

    WBServer();
    ~WBServer();

    void init_websocket_options();
    void init_websocket_behavior();
    
    void init_websocket_ssl_server();
    void init_websocket_server();

    void on_open(uWS::WebSocket<false, true> * );

    void on_message(uWS::WebSocket<false, true> *, std::string_view, uWS::OpCode);

    void on_ping(uWS::WebSocket<false, true> * );

    void on_pong(uWS::WebSocket<false, true> * );

    void on_close(uWS::WebSocket<false, true> * );

    void listen();

    void set_front_server(FrontServer*);

    void launch();

    void release();

    void broadcast(string msg);

    private:
        us_socket_context_options_t             socket_options_;
        uWS::App::WebSocketBehavior             websocket_behavior_;

        uWS::App                                wss_server_;

        int                                     server_port_{9002};
        std::set<uWS::WebSocket<false, true> *> wss_con_set_;

        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_{nullptr};
};

FORWARD_DECLARE_PTR(WBServer);
