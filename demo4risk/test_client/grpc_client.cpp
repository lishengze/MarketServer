#include "grpc_client.h"
#include "Log/log.h"
#include "global_declare.h"

#include <grpcpp/grpcpp.h>

void GrpcClient::get_trade_data(const ReqTradeInfoLocal& req_info)
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
                    + ", price: " + std::to_string(reply.mutable_price()->value() / pow(10, reply.mutable_price()->precise() ) ) 
                    + ", volume: " +  std::to_string(reply.mutable_volume()->value() / pow(10, reply.mutable_volume()->precise() )));

            LOG_INFO("[PRICE]: value: " + std::to_string(reply.mutable_price()->value()) 
                + ", precise: " + std::to_string(reply.mutable_price()->precise()) );
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

        grpc::Status status = stub->RequestTradeData(&context, request, &reply);

        if (status.ok())
        {
            LOG_INFO(reply.exchange() + "." +reply.symbol() 
                    + ", price: " + std::to_string(reply.mutable_price()->value() / pow(10, reply.mutable_price()->precise() ) ) 
                    + ", volume: " +  std::to_string(reply.mutable_volume()->value() / pow(10, reply.mutable_volume()->precise() )));

            LOG_INFO("[PRICE]: value: " + std::to_string(reply.mutable_price()->value()) 
                + ", precise: " + std::to_string(reply.mutable_price()->precise()) );
        }

        // switch(channel->GetState(true)) {
        //     case GRPC_CHANNEL_IDLE: {
        //         LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_IDLE");
        //         break;
        //     }
        //     case GRPC_CHANNEL_CONNECTING: {                
        //         LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_CONNECTING");
        //         break;
        //     }
        //     case GRPC_CHANNEL_READY: {           
        //         LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_READY");
        //         break;
        //     }
        //     case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
        //         LOG_WARN("GetStreamTradeData: status is GRPC_CHANNEL_TRANSIENT_FAILURE");
        //         return;
        //     }
        //     case GRPC_CHANNEL_SHUTDOWN: {        
        //         LOG_INFO("GetStreamTradeData: status is GRPC_CHANNEL_SHUTDOWN");
        //         break;
        //     }
        // }


    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string get_otc_failed_info(quote::service::v1::QuoteResponse_Result result) {
    if (result == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_WRONG_SYMBOL) {
        return "wrong_symbol";
    } else if (result == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_WRONG_DIRECTION) {
        return "wrong_direction";
    } else if (result == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_NOT_ENOUGH_VOLUME) {
        return "not_enough_volume";
    } else if (result == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_NOT_ENOUGH_AMOUNT) {
        return "not_enough_amount";
    }
}

void OTCClient::otc_(string symbol, int data_count) {

        LOG_INFO("request_trade_data");

        for (int i = 0; i<data_count; ++i) {
            std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());

            std::unique_ptr<RiskController::Stub> stub = RiskController::NewStub(channel);

            QuoteRequest  request;
            QuoteResponse reply;

            request.set_symbol(symbol);
            request.set_turnover(100);

            LOG_INFO("Request: " + symbol + ", 100 usdt");


            request.set_direction(quote::service::v1::QuoteRequest_Direction::QuoteRequest_Direction_BUY);

            ClientContext context;

            grpc::Status status = stub->OtcQuote(&context, request, &reply);  

            if (reply.result() == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_OK) {
                LOG_INFO("Reply Price: " + reply.price());
            } else {
                LOG_WARN("Request"  + symbol + ", 100 usdt Failed! " + get_otc_failed_info(reply.result()));
            }


            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
}