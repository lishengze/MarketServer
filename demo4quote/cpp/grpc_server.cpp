#include "stream_engine_config.h"
#include "grpc_server.h"

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

    caller_demo_ = new GrpcCall<GrpcDemoEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_demo_;
    call_id++;

    caller_subscribe_single_ = new GrpcCall<SubscribeSingleQuoteEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_subscribe_single_;
    call_id++;

    caller_subscribe_mix_ = new GrpcCall<SubscribeMixQuoteEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_subscribe_mix_;
    call_id++;

    caller_setparams_ = new GrpcCall<SetParamsEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_setparams_;
    call_id++;

    caller_getparams_ = new GrpcCall<GetParamsEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_getparams_;
    call_id++;

    caller_getklines_ = new GrpcCall<GetKlinesEntity>(call_id, &service_, cq_.get(), provider_);
    callers_[call_id] = caller_getklines_;
    call_id++;

    caller_getlast_ = new GrpcCall<GetLastEntity>(call_id, &service_, cq_.get(), provider_);
    callers_[call_id] = caller_getlast_;
    call_id++;
}

void ServerEndpoint::publish_single(const string& exchange, const string& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update)
{
    SnapAndUpdate data;
    data.snap = snap;
    data.update = update;
    caller_subscribe_single_->add_data(data);
    //std::cout << "publish_single finish " << exchange << " " << symbol << std::endl;
};

void ServerEndpoint::publish_mix(const string& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update)
{
    SnapAndUpdate data;
    data.snap = snap;
    data.update = update;
    caller_subscribe_mix_->add_data(data);
};

void ServerEndpoint::_handle_rpcs() 
{
    std::cout << "_handle_rpcs running on ..." << std::endl;
    void* tag;
    bool ok;
    while(true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        if( ok ) {
            BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
            CommonGrpcCall* caller = callers_[cd->call_id_];
            caller->process(cd);
        } else {
            BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
            CommonGrpcCall* caller = callers_[cd->call_id_];
            caller->release(cd);
        }
    }
}

void ServerEndpoint::on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines)
{
    for( const auto& v : klines ) {
        _log_and_print("%s index=%lu open=%s high=%s low=%s close=%s", symbol.c_str(), 
            v.index,
            v.px_open.get_str_value().c_str(),
            v.px_high.get_str_value().c_str(),
            v.px_low.get_str_value().c_str(),
            v.px_close.get_str_value().c_str()
            );            

        WrapperKlineData tmp;
        vassign(tmp.exchange, 16, "bcts");
        vassign(tmp.symbol, 16, symbol.c_str());
        vassign(tmp.resolution, resolution);
        vassign(tmp.index, v.index);
        vassign(tmp.px_open, v.px_open);
        vassign(tmp.px_high, v.px_high);
        vassign(tmp.px_low, v.px_low);
        vassign(tmp.px_close, v.px_close);
        vassign(tmp.volume, v.volume);
        caller_getlast_->add_data(tmp);
    }
}

