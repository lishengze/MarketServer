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
#include "../data_structure/data_struct.h"
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

    void broadcast(string msg);

    void broadcast_enhanced_data(string symbol, string data_str);
    
    void process_on_message(string ori_msg, WebsocketClass * ws);
    
    void process_sub_info(string ori_msg, WebsocketClass * ws);

    void process_kline_data(string ori_msg, WebsocketClass* ws);

    void process_heartbeat(WebsocketClass* ws);

    void clean_client(WebsocketClassThreadSafePtr ws);

    void send_data(ID_TYPE id, string msg);

    void send_data(WebsocketClassThreadSafePtr ws, string msg);

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

        std::set<WebsocketClassThreadSafePtr, 
              LessWebsocketClassThreadSafePtr>  wss_con_set_;

        std::mutex                              wss_map_mutex_;
        std::map<int, WebsocketClassThreadSafePtr> wss_map_;
 
        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_{nullptr};

        boost::shared_ptr<std::thread>          heartbeat_thread_{nullptr};    

        int                                     heartbeat_seconds_{5};    

        std::mutex                              heartbeat_mutex_;

        std::atomic_ullong                      socket_id_{0};
};

FORWARD_DECLARE_PTR(WBServer);
