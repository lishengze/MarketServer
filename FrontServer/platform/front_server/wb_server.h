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

    void launch();

    private:
        us_socket_context_options_t             socket_options_;
        uWS::App::WebSocketBehavior             websocket_behavior_;        
        boost::shared_ptr<uWS::SSLApp>          ssl_app_;
        uWS::App                                wss_server_;
        int                                     server_port_{9000};

        std::set<uWS::WebSocket<false, true> *> wss_con_set_;
};

FORWARD_DECLARE_PTR(WBServer);
