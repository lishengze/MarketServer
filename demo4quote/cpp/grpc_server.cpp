#include "grpc_server.h"

void GrpcServer::publish_single(const string& exchange, const string& symbol, std::shared_ptr<QuoteData> snap, std::shared_ptr<QuoteData> update)
{
    caller_subscribe_single_->add_data(snap, update);
    //std::cout << "publish_single finish " << exchange << " " << symbol << std::endl;
};

void GrpcServer::publish_mix(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update)
{
    caller_subscribe_mix_->add_data(snap, update);
};

void GrpcServer::_handle_rpcs() {
    unordered_map<int, CommonGrpcCall*> callers;
    int call_id = 0;

    caller_demo_ = new GrpcCall<GrpcDemoEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_demo_;
    call_id++;

    caller_subscribe_single_ = new GrpcCall<SubscribeSingleQuoteEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_subscribe_single_;
    call_id++;

    caller_subscribe_mix_ = new GrpcCall<SubscribeMixQuoteEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_subscribe_mix_;
    call_id++;

    caller_setparams_ = new GrpcCall<SetParamsEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_setparams_;
    call_id++;

    caller_getparams_ = new GrpcCall<GetParamsEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_getparams_;
    call_id++;

    void* tag;
    bool ok;
    while(true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        if( ok ) {
            BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
            CommonGrpcCall* caller = callers[cd->call_id_];
            caller->process(cd);
        } else {
            BaseGrpcEntity* cd = static_cast<BaseGrpcEntity*>(tag);
            CommonGrpcCall* caller = callers[cd->call_id_];
            caller->release(cd);
        }
    }
}
