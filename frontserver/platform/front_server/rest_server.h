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

        void process_get(uWS::HttpResponse<false> *, uWS::HttpRequest *);

        void process_post(uWS::HttpResponse<false> *, uWS::HttpRequest *);

        void process_del(uWS::HttpResponse<false> *, uWS::HttpRequest *);

        void process_put(uWS::HttpResponse<false> *, uWS::HttpRequest *);

        void set_front_server(FrontServer* front_server);

    private:

        boost::shared_ptr<uWS::App>             rest_server_{nullptr};

        int                                     server_port_{9001};

        boost::shared_ptr<std::thread>          listen_thread_;

        FrontServer*                            front_server_;
};

FORWARD_DECLARE_PTR(RestServer);