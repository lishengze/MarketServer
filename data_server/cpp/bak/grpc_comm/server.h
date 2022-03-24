#pragma once

#include "../global_declare.h"
#include "../data_struct/data_struct.h"
#include "rpc.h"

FORWARD_DECLARE_PTR(ServerEngine);

class GrpcServer
{
public:
    GrpcServer(string address, ServerEngine* server_engine=nullptr):address_{address}, server_engine_{server_engine} {}

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
    ServerEngine*                                 server_engine_;

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