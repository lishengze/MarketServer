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
#include "../data_process/data_struct.h"
#include <thread>

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
    using websocket_class = uWS::WebSocket<false, true>;
    
    public:

    WBServer();
    ~WBServer();

    void init_websocket_options();
    void init_websocket_behavior();
    
    void init_websocket_ssl_server();
    void init_websocket_server();

    void on_open(websocket_class * );

    void on_message(websocket_class *, std::string_view, uWS::OpCode);

    void on_ping(websocket_class * );

    void on_pong(websocket_class * );

    void on_close(websocket_class * );

    void listen();

    void set_front_server(FrontServer*);

    void launch();

    void release();

    void broadcast(string msg);

    void broadcast_enhanced_data(EnhancedDepthData& en_depth_data);

    void process_sub_info(string ori_msg, websocket_class * ws);

    void clean_client(websocket_class * ws);

    void heartbeat_run();

    void start_heartbeat();

    void check_heartbeat();

    private:
        us_socket_context_options_t             socket_options_;

        uWS::App::WebSocketBehavior             websocket_behavior_;

        uWS::App                                wss_server_;

        int                                     server_port_{9002};
        std::set<websocket_class *> wss_con_set_;

        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_{nullptr};

        std::map<string, std::set<websocket_class *>> ws_sub_map_;

        boost::shared_ptr<std::thread>          heartbeat_thread_{nullptr};    

        int                                     heartbeat_seconds_{5};                         
};

FORWARD_DECLARE_PTR(WBServer);
