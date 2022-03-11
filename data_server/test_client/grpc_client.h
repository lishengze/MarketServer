#pragma once
#include "global_declare.h"


struct ReqTradeInfoLocal
{
    string exchange;
    string symbol;
    uint64 time;
};


class GrpcClient
{
    public:
        GrpcClient(string address):address_{address} {}

        void start();

        void req_thread_main();

        void request_trade_data(const ReqTradeInfoLocal& req_info);

    private:
        string          address_;
        std::thread     req_thread_;
};
