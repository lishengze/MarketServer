#include "grpc_client.h"
#include "Log/log.h"
#include "global_declare.h"

#include <grpcpp/grpcpp.h>

#define OTC_BUY quote::service::v1::QuoteRequest_Direction::QuoteRequest_Direction_BUY
#define OTC_SELL quote::service::v1::QuoteRequest_Direction::QuoteRequest_Direction_SELL

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

string get_otc_direction(quote::service::v1::QuoteRequest_Direction direction) {
    if (direction == quote::service::v1::QuoteRequest_Direction::QuoteRequest_Direction_BUY) {
        return "buy";
    } else if(direction == quote::service::v1::QuoteRequest_Direction::QuoteRequest_Direction_SELL) {
        return "Sell";
    }
}

string get_otc_req_info(string symbol, double amount, quote::service::v1::QuoteRequest_Direction direction) { 
    return string("Request: " + symbol + ", "+ std::to_string(amount) +" usdt, direction: " + get_otc_direction(direction));
}

void OTCClient::OTC() {
    std::vector<string> symbol_list{"BTC_USDT", "ETH_USDT"}; 
    int symbol_index = 0;
    while (true) {

        string cur_symbol = symbol_list[symbol_index % symbol_list.size()];
        
        QuoteResponse buy_result;
        QuoteResponse sell_result;
        double amount = 100;

        if (!otc_(cur_symbol, amount, OTC_BUY, buy_result)) continue;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!otc_(cur_symbol, amount, OTC_SELL, sell_result)) continue;

        if (std::stof(buy_result.price()) < std::stof(sell_result.price())) {
            LOG_ERROR(get_otc_req_info(cur_symbol, amount, OTC_BUY) + " buy_result: " + buy_result.price() + " \nbigger than \n" 
                    + get_otc_req_info(cur_symbol, amount, OTC_BUY)+ " sell_result: " + sell_result.price());
        }

        symbol_index++;

        std::this_thread::sleep_for(std::chrono::seconds(4));
    }
}

bool OTCClient::otc_(string symbol, double amount, quote::service::v1::QuoteRequest_Direction direction, QuoteResponse& reply) {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());

        std::unique_ptr<RiskController::Stub> stub = RiskController::NewStub(channel);

        QuoteRequest  request;

        request.set_symbol(symbol);
        request.set_turnover(amount);
        request.set_direction(direction);

        string req_info_str = get_otc_req_info(symbol, amount, direction);

        ClientContext context;

        grpc::Status status = stub->OtcQuote(&context, request, &reply);  

        if (reply.result() == quote::service::v1::QuoteResponse_Result::QuoteResponse_Result_OK) {
            LOG_INFO(req_info_str + ", Reply Price: " + reply.price());
            return true;
        } else {
            LOG_WARN(req_info_str + " Failed! Msg: " + get_otc_failed_info(reply.result()));
            return false;
        }        
}