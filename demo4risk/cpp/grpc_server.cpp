#include "grpc_server.h"

void GrpcServer::init(const string& grpc_addr)
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

    caller_marketstream4broker_ = new GrpcCall<MarketStream4BrokerEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_marketstream4broker_;
    call_id++;

    caller_marketstream4hedge_ = new GrpcCall<MarketStream4HedgeEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_marketstream4hedge_;
    call_id++;
    
    caller_marketstream4client_ = new GrpcCall<MarketStream4ClientEntity>(call_id, &service_, cq_.get());
    callers_[call_id] = caller_marketstream4client_;
    call_id++;
}

void GrpcServer::publish4Hedge(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update)
{
    caller_marketstream4hedge_->add_data(snap, update);
}

void GrpcServer::publish4Broker(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update)
{
    caller_marketstream4broker_->add_data(snap, update);
}

void GrpcServer::publish4Client(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update)
{
    caller_marketstream4client_->add_data(snap, update);
}

void GrpcServer::_handle_rpcs() 
{
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