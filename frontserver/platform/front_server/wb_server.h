#pragma once

#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <string_view>

#include "App.h"
#include "libusockets.h"

#include <vector>
#include <set>
#include <mutex>

#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"
#include <thread>
#include <atomic>

using std::string;
using std::cout;
using std::endl;
using std::vector;

class FrontServer;

class WBServer
{       
    public:

    WBServer();
    ~WBServer();

    void init_websocket_options();
    void init_websocket_behavior();
    
    void init_websocket_ssl_server();
    void init_websocket_server();

    void on_open(WebsocketClass * );

    void on_message(WebsocketClass *, std::string_view, uWS::OpCode);

    void on_ping(WebsocketClass * );

    void on_pong(WebsocketClass * );

    void on_close(WebsocketClass * );

    void listen();

    void set_front_server(FrontServer*);

    void launch();

    void release();

    void process_on_open(WebsocketClass * ws);

    void process_on_message(string ori_msg, WebsocketClass * ws);
    
    void process_depth_req(string ori_msg, ID_TYPE socket_id);

    void process_kline_req(string ori_msg, ID_TYPE socket_id);

    void process_heartbeat(ID_TYPE socket_id);

    WebsocketClassThreadSafePtr store_ws(WebsocketClass * ws);

    bool check_ws(WebsocketClass * ws);

    void clean_ws(WebsocketClass* ws);

    bool send_data(ID_TYPE socket_id, string msg);

    void heartbeat_run();

    void start_heartbeat();

    void check_heartbeat();

    string get_error_send_rsp_string(string err_msg);

    string get_heartbeat_str();

    private:
        us_socket_context_options_t             socket_options_;

        uWS::App::WebSocketBehavior             websocket_behavior_;

        uWS::App                                wss_server_;

        int                                     server_port_{9002};

        std::map<ID_TYPE, WebsocketClassThreadSafePtr> wss_con_map_;

        std::mutex                              wss_con_set_mutex_;
 
        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_{nullptr};

        boost::shared_ptr<std::thread>          heartbeat_thread_{nullptr};    

        int                                     heartbeat_seconds_{5};    

        std::mutex                              heartbeat_mutex_;

        std::atomic_ullong                      socket_id_{0};
};

FORWARD_DECLARE_PTR(WBServer);
