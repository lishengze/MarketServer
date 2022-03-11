#include "grpc_client.h"
#include "Log/log.h"
#include "global_declare.h"

#include <grpcpp/grpcpp.h>

void GrpcClient::start()
{
    try
    {
        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void GrpcClient::request_trade_data(const ReqTradeInfoLocal& req_info)
{
    try
    {
        LOG_INFO("request_trade_data");
        
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());

        std::unique_ptr<MarketService::Stub> stub = MarketService::NewStub(channel);

        ReqTradeInfo request;
        TradeData    reply;

        request.set_exchange(req_info.exchange);
        request.set_symbol(req_info.symbol);
        request.set_time(req_info.time);

        ClientContext context;

        // The actual RPC.
        std::unique_ptr<grpc::ClientReader<TradeData>> reader = stub->GetStreamTradeData(&context, request);

        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_IDLE");
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_CONNECTING");
                break;
            }
            case GRPC_CHANNEL_READY: {           
                LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_READY");
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                LOG_WARN("GetStreamTradeData: status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_SHUTDOWN");
                break;
            }
        }

        while(reader->Read(&reply))
        {
            LOG_INFO(reply.exchange() + "." +reply.symbol() 
                    + ", price: " + std::to_string(reply.price().value()) 
                    + ", volume: " +  std::to_string(reply.volume().value()));
        }

        Status status = reader->Finish();
        if (status.ok()) {
            LOG_INFO("GetStreamTradeData rpc finish succeeded.");
        } else {
            LOG_WARN("GetStreamTradeData rpc finish failed.");
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}