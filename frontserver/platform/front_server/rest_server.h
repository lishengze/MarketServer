#pragma once

#include <thread>
#include "../front_server_declare.h"
#include "../data_structure/base_data.h"

#include "App.h"
#include "libusockets.h"
#include "pandora/util/thread_basepool.h"

class FrontServer;

class RestServer:public utrade::pandora::ThreadBasePool
{
    public:
        RestServer(utrade::pandora::io_service_pool& pool);
        
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

        bool process_v1_request_enquiry(string& query_param, string& err_msg, HttpResponse * res);

        void send_err_msg(HttpResponse *, string msg);

        void test_response_multithread_run(HttpResponse * rsp);

        void test_response_multithread(HttpResponse * rsp);

        void process_main();

        // void httpResponseOnAborted();

    private:
        int                                     server_port_{9001};

        boost::shared_ptr<std::thread>          listen_thread_;

        std::map<ID_TYPE, HttpResponseThreadSafePtr> response_map_;
        std::mutex                                   response_map_mutex_;

        FrontServer*                            front_server_;


        static string                           PARMA_SPLIT_CHAR;
        static string                           QUERY_START_CHAR;
        static string                           QUERY_SPLIT_CHAR;

        static string                           VERSION1;
        static string                           VERSION2;
        static string                           KLINE_REQUEST;
        static string                           KLINE_REQUEST_SYMBOL;
        static string                           KLINE_REQUEST_STARTTIME;
        static string                           KLINE_REQUEST_ENDTIME;
        static string                           KLINE_REQUEST_FREQUENCY;

        static string                           ENQUIRY_REQUEST;
        static string                           ENQUIRY_REQUEST_SYMBOL;
        static string                           ENQUIRY_REQUEST_TYPE;
        static string                           ENQUIRY_REQUEST_VOLUME;
        static string                           ENQUIRY_REQUEST_AMOUNT;

        std::thread                             process_thread_;

};

FORWARD_DECLARE_PTR(RestServer);