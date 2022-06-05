#pragma once
#include "global_declare.h"


struct ReqTradeInfoLocal
{
    string exchange;
    string symbol;
    uint64 time;
};

class BaseGrpcClient
{
    public:
        BaseGrpcClient(string address):address_{address} {}

        virtual ~BaseGrpcClient() {
            if (req_thread_.joinable()) {
                req_thread_.join();
            }
        }

        virtual void start() {}

        virtual void req_thread_main() {}


    protected:
        string          address_;
        std::thread     req_thread_;
};


class GrpcClient:public BaseGrpcClient
{
    public:
        GrpcClient(string address):BaseGrpcClient{address} {}

        void get_trade_data(const ReqTradeInfoLocal& req_info);

        void request_trade_data(const ReqTradeInfoLocal& req_info);

    private:
        string          address_;
        std::thread     req_thread_;
};


class OTCClient:public BaseGrpcClient
{
    public:
        OTCClient(string address):BaseGrpcClient{address} {}

        void OTC();

        bool otc_(string symbol, double amount, quote::service::v1::QuoteRequest_Direction direction, QuoteResponse& reply);
};


