#pragma once

#include "../global_declare.h"
#include "rpc.h"

class GrpcServer
{
public:
    GrpcServer(string address):address_{address} {}

    virtual ~GrpcServer();

    void start();

    void init_async_server_env();

    void init_rpc();

    void init_cq_thread();

    void run_cq_loop();

public:
    void reconnect_rpc(BaseRPC* rpc);

private:
    string                                        address_;

    std::unique_ptr<grpc::ServerCompletionQueue>  cq_;
    std::unique_ptr<grpc::Server>                 server_;
    grpc::ServerBuilder                           builder_;
    MarketService::AsyncService                   service_;
    std::thread                                   cq_thread_;

private:
    RequestTradeDataRPC*                          request_trade_data_rpc_{nullptr};
    GetTradeStreamDataRPC*                        get_stream_trade_data_rpc{nullptr};
};