#pragma once

#include "../global_declare.h"

class BaseRPC
{
public:
    BaseRPC(grpc::ServerCompletionQueue* cq, MarketService* service):
        cq_{cq}, service_{service}
    {
        
    }

    virtual ~BaseRPC() {}

    virtual void start();

    virtual void process();

    virtual void start_monitor_request() { }

    virtual void proceed();

    virtual BaseRPC* spawn();

    virtual void create_rpc_for_next_client();


public:
    std::string                      rpc_name_{"BaseRpc"};
    bool                             is_stream_{false};
    bool                             is_first_process_{true};
    
public:
    grpc::ServerCompletionQueue*        cq_{nullptr};    
    Proto3::MarketData::MarketService*  service_;

};

class RequestTradeDataRPC: public BasePRC
{
    public:
        RequestTradeDataRPC(grpc::ServerCompletionQueue* cq, MarketService* service):
                            BasePRC{cq, service}
        {

        }

        virtual void start_monitor_request();

        virtual BaseRPC* spawn();
};
