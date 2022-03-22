#pragma once

#include "global_declare.h"

FORWARD_DECLARE_PTR(GrpcServer);
FORWARD_DECLARE_PTR(DBEnginePool);

class ServerEngine
{
    public:
        ServerEngine();

        void start();

        static volatile int signal_sys;
        static void signal_handler(int signum);

        bool get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data);

    private:
        GrpcServerPtr       grpc_server_sptr_{nullptr};
        DBEnginePoolPtr     db_engine_sptr_{nullptr};
};