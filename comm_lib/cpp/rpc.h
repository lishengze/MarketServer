#pragma once

#include "comm_declare.h"
#include "comm_log.h"

COMM_NAMESPACE_START

class GrpcServer;

class BaseRPC
{
public:
    BaseRPC(MarketService::AsyncService* service, grpc::ServerCompletionQueue* cq, string rpc_name):
        service_{service}, cq_{cq}, rpc_name_{rpc_name}
    {
        ++RPC_ID;

        rpc_id = RPC_ID;

        COMM_LOG_INFO("Create " + rpc_info());
    }

    virtual ~BaseRPC() {}

    virtual void register_server(GrpcServer* server) {server_ = server;}

    virtual void start();

    virtual void process();

    virtual void start_monitor_request() {}

    virtual void proceed() {}

    virtual BaseRPC* spawn();

    virtual void create_rpc_for_next_client();

    virtual string get_rpc_name() { return rpc_name_;}

    string rpc_info() { return rpc_name_ + "_" + std::to_string(rpc_id);}


public:
    std::string                      rpc_name_{"BaseRpc"};
    bool                             is_stream_{false};

    bool                             is_first_connect_{true};
    bool                             is_inner_cq_event_{false};
    bool                             is_finished_{false};

    GrpcServer*                      server_{nullptr};



    static uint64                    RPC_ID;

    uint64                           rpc_id;

public:
    Proto3::MarketData::MarketService::AsyncService*  service_;
    grpc::ServerCompletionQueue*                      cq_{nullptr};    
};

class RequestTradeDataRPC: public BaseRPC
{
    public:
        RequestTradeDataRPC(grpc::ServerCompletionQueue* cq, MarketService::AsyncService* service):
                            BaseRPC{service, cq, "RequestTradeDataRPC"}, responder_{&context_}
        {

        }

        virtual void start_monitor_request();

        virtual BaseRPC* spawn();

        virtual void proceed();

    private:
        ServerContext                            context_;

        ReqTradeInfo                             request_info_;
        PTradeData                               reply_info_;
        ServerAsyncResponseWriter<PTradeData>    responder_;
        
};  

class GetTradeStreamDataRPC: public BaseRPC
{
    public:
        GetTradeStreamDataRPC(grpc::ServerCompletionQueue* cq, MarketService::AsyncService* service):
                              BaseRPC{service, cq, "GetTradeStreamDataRPC"}, responder_{&context_}
        {

        }

        virtual void start_monitor_request();

        virtual BaseRPC* spawn();

        virtual void proceed();

    private:
        ServerContext                            context_;

        ReqTradeInfo                             request_info_;
        PTradeData                               reply_info_;
        ServerAsyncWriter<PTradeData>            responder_;

        // ServerAsyncResponse
        
};  

COMM_NAMESPACE_END