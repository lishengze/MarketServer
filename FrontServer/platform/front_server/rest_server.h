#pragma once

#include <thread>
#include "../front_server_declare.h"

#include "App.h"
#include "libusockets.h"

class RestServer
{
    RestServer();
    ~RestServer();

    void launch();

    void listen();

    void release();

    void process_get(uWS::HttpResponse<false> *, uWS::HttpRequest *);

    void process_post(uWS::HttpResponse<false> *, uWS::HttpRequest *);

    void process_del(uWS::HttpResponse<false> *, uWS::HttpRequest *);

    void process_put(uWS::HttpResponse<false> *, uWS::HttpRequest *);

    private:

        boost::shared_ptr<uWS::App>             rest_server_{nullptr};

        int                                     server_port_{9001};

        std::set<uWS::WebSocket<false, true> *> wss_con_set_;

        boost::shared_ptr<std::thread>          listen_thread_;
    };
