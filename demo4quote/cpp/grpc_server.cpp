#include "stream_engine_config.h"
#include "grpc_server.h"
#include "pandora/util/time_util.h"

#include <iostream>

void ServerEndpoint::init(const string& grpc_addr)
{
    std::string server_address(grpc_addr);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    
    int call_id = 0;

    //caller_subscribe_single_ = new GrpcCall<SubscribeSingleQuoteEntity>(call_id, &service_, cq_.get(), quote_cacher_);

    //caller_subscribe_mix_ = new GrpcCall<SubscribeMixQuoteEntity>(call_id, &service_, cq_.get(), quote_cacher_);

    caller_getparams_ = new GrpcCall<GetParamsEntity>(call_id, &service_, cq_.get());

    caller_getklines_ = new GrpcCall<GetKlinesEntity>(call_id, &service_, cq_.get(), cacher_);

    caller_getlast_ = new GrpcCall<GetLastEntity>(call_id, &service_, cq_.get(), cacher_);

    //caller_subscribe_trade_ = new GrpcCall<SubscribeTradeEntity>(call_id, &service_, cq_.get());

    caller_getlast_trades_ = new GrpcCall<GetLastTradesEntity>(call_id, &service_, cq_.get(), quote_cacher_);

    caller_subscribe_in_binary_ = new GrpcCall<SubscribeQuoteInBinaryEntity>(call_id, &service_, cq_.get(), quote_cacher_);
}

void ServerEndpoint::publish_binary(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap)
{
    // cout << "ServerEndpoint::publish_binary Start" << endl;

    string tmp;
    if( !snap->SerializeToString(&tmp) )
    {
        cout << "[Error] ServerEndpoint::publish_binary SerializeToString Failed" << endl;
        return;
    }
    else
    {
        cout << "Serialized data: " << exchange << "." << symbol << "  len: " << tmp.length() << endl;
    }
        
    
    uint32 length = tmp.size();
    uint32 data_type = QUOTE_TYPE_DEPTH;
    string data = tfm::format("%u;%u;", length, data_type);
    data.insert(data.end(), tmp.begin(), tmp.end());

    // cout << "ServerEndpoint::publish_binary " << exchange << " " << symbol << endl;

    caller_subscribe_in_binary_->add_data(exchange, symbol, data);
}
/*
void ServerEndpoint::publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap)
{
    // streamengine生成时间，这个值在发送之前再做一次判断
    type_tick now = get_miliseconds();
    //tfm::printfln("publish_single %u", now);
    snap->set_time_produced_by_streamengine(now);
    caller_subscribe_single_->add_data(snap);
    //std::cout << "publish_single finish " << exchange << " " << symbol << std::endl;
};

void ServerEndpoint::publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap)
{
    caller_subscribe_mix_->add_data(snap);
};
*/
void ServerEndpoint::publish_trade(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<TradeWithDecimal> trade)
{
    //caller_subscribe_trade_->add_data(trade);
    
    string tmp;
    if( !trade->SerializeToString(&tmp) )
        return;
    
    uint32 length = tmp.size();
    uint32 data_type = QUOTE_TYPE_TRADE;
    string data = tfm::format("%u;%u;", length, data_type);
    data.insert(data.end(), tmp.begin(), tmp.end());

    caller_subscribe_in_binary_->add_data(exchange, symbol, data);
};

void ServerEndpoint::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines)
{
    WrapperKlineData tmp;
    vassign(tmp.exchange, exchange);
    vassign(tmp.symbol, symbol);
    vassign(tmp.resolution, resolution);
    tmp.klines = klines;
    
    // cout << "ServerEndpoint::on_kline add_data " << exchange << " " << symbol << " " << resolution << " " << klines.size() << endl;

    caller_getlast_->add_data(tmp);

    /*
    for( const auto& v : klines ) 
    {
        
        _log_and_print("publish %s.%s resolution=%d index=%lu open=%s high=%s low=%s close=%s", exchange, symbol, resolution,
            v.index,
            v.px_open.get_str_value(),
            v.px_high.get_str_value(),
            v.px_low.get_str_value(),
            v.px_close.get_str_value()
            );
    }
    */
}

void ServerEndpoint::_handle_rpcs() 
{
    std::cout << "_handle_rpcs running on ..." << std::endl;
    void* tag;
    bool ok;
    
    int64 loop_id = 0;

    while(true) {
        //tfm::printfln("cq->next");
        // GPR_ASSERT();

        bool result = cq_->Next(&tag, &ok);

        // std::cout << "result: " << result << ", ok: " << ok << std::endl;
        
        type_tick begin = get_miliseconds();
        BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
        if( ok ) {

            // std::cout << "cq: " << cd->get_entity_name() <<" " << utrade::pandora::NanoTimeStr() << endl;

            cd->proceed(loop_id);
        } else {
            BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
            cd->release(loop_id);
        }

        type_tick end = get_miliseconds();
        // if( (end - begin) >= 10 )
            // tfm::printfln("grpc-loop[%d] handle %d cost %u", loop_id, cd->call_id_, end - begin);
    }
}
