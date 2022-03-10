#pragma once

#include "../global_declare.h"

using Proto3::MarketData::ReqTradeInfo;
using Proto3::MarketData::TradeData;

using grpc::ServerAsyncWriter;
using grpc::ServerContext;




class BaseRPC
{
public:
    BaseRPC(grpc::ServerCompletionQueue* cq, MarketService::AsyncService* service):
        cq_{cq}, service_{service}
    {
        
    }

    virtual ~BaseRPC() {}

    virtual void start();

    virtual void process();

    virtual void start_monitor_request() {}

    virtual void proceed() {}

    virtual BaseRPC* spawn();

    virtual void create_rpc_for_next_client();


public:
    std::string                      rpc_name_{"BaseRpc"};
    bool                             is_stream_{false};
    bool                             is_first_process_{true};
    
public:
    grpc::ServerCompletionQueue*        cq_{nullptr};    
    Proto3::MarketData::MarketService::AsyncService*  service_;

};

class RequestTradeDataRPC: public BaseRPC
{
    public:
        RequestTradeDataRPC(grpc::ServerCompletionQueue* cq, MarketService::AsyncService* service):
                            BaseRPC{cq, service}, responder_{&context_}
        {

        }

        virtual void start_monitor_request();

        virtual BaseRPC* spawn();

    private:
        ReqTradeInfo                             request_info_;
        TradeData                                reply_info_;

        ServerContext                            context_;
        ServerAsyncResponseWriter<TradeData>     responder_;
        
};  
