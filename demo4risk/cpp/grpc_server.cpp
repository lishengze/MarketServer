#include "grpc_server.h"

void GrpcServer::publish(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update)
{
    caller_marketstream_->add_data(snap, update);
};

void GrpcServer::_handle_rpcs() {
    unordered_map<int, CommonGrpcCall*> callers;
    int call_id = 0;

    caller_marketstream_ = new GrpcCall<MarketStreamEntity>(call_id, &service_, cq_.get());
    callers[call_id] = caller_marketstream_;
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