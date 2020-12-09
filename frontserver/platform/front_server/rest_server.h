#pragma once

#include <thread>
#include "../front_server_declare.h"

#include "App.h"
#include "libusockets.h"

class FrontServer;

class RestServer
{
    public:
        RestServer();
        
        ~RestServer();

        void launch();

        void listen();

        void release();

        void process_get(HttpResponse *, HttpRequest *);

        void process_post(HttpResponse *, HttpRequest *);

        void process_del(HttpResponse *, HttpRequest *);

        void process_put(HttpResponse *, HttpRequest *);

        void set_front_server(FrontServer* front_server);

        bool process_v1_request_kline(string& query_param, string& err_msg, HttpResponse *, HttpRequest *);


    private:
        int                                     server_port_{9001};

        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_;

        static string                           VERSION1;
        static string                           VERSION2;
        static string                           KLINE_REQUEST;
        static string                           KLINE_REQUEST_SYMBOL;
        static string                           KLINE_REQUEST_STARTTIME;
        static string                           KLINE_REQUEST_ENDTIME;
        static string                           KLINE_REQUEST_FREQUENCY;
};

FORWARD_DECLARE_PTR(RestServer);