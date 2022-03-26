#pragma once

#include "global_declare.h"
#include "data_struct/data_struct.h"
#include "comm_interface_define.h"
#include "rpc_server.h"
#include "comm.h"
#include "db_engine/db_engine.h"

USING_COMM_NAMESPACE

// FORWARD_DECLARE_PTR(GrpcServer);
// FORWARD_DECLARE_PTR(DBEnginePool);

// class ServerEngine
// {
//     public:
//         ServerEngine();

//         void start();

//         bool get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data);

//         void insert_kline_data(const KlineData& kline_data);

//         virtual void on_kline( KlineData& kline);

//         virtual void on_trade( TradeData& trade);

//     public:
//         static volatile int signal_sys;
//         static void signal_handler(int signum);    
// };

/**/
class ServerEngine:public bcts::comm::QuoteSourceCallbackInterface, public bcts::comm::ServerEngineInterface
{
    public:
        ServerEngine();

        void start();



        virtual bool get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data);

        void insert_kline_data(const KlineData& kline_data);

        virtual void on_kline( KlineData& kline);

        virtual void on_trade( TradeData& trade);

    private:
        GrpcServerPtr           grpc_server_sptr_{nullptr};
        DBEnginePoolPtr         db_engine_sptr_{nullptr};
        bcts::comm::CommPtr     comm_server_sptr{nullptr};

    public:
        static volatile int signal_sys;
        static void signal_handler(int signum);    
};

