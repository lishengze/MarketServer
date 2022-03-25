#pragma once

#include "comm_declare.h"
#include "comm_interface_define.h"
#include "rpc.h"
#include "util.h"


COMM_NAMESPACE_START

class GrpcServer
{
public:
    GrpcServer(string address, GrpcServerInterface* server_engine=nullptr):address_{address}, server_engine_{server_engine} {
        init_log();
    }

    virtual ~GrpcServer();

    void start();

    void init_async_server_env();

    void init_rpc();

    void init_cq_thread();

    void run_cq_loop();

    bool get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data);

public:
    void reconnect_rpc(BaseRPC* rpc);

private:
    string                                        address_;
    GrpcServerInterface*                          server_engine_;

    std::unique_ptr<grpc::ServerCompletionQueue>  cq_;
    std::unique_ptr<grpc::Server>                 server_;
    grpc::ServerBuilder                           builder_;
    PMarketService                                service_;
    std::thread                                   cq_thread_;

private:
    RequestTradeDataRPC*                          request_trade_data_rpc_{nullptr};
    GetTradeStreamDataRPC*                        get_stream_trade_data_rpc{nullptr};
};

DECLARE_PTR(GrpcServer);

COMM_NAMESPACE_END