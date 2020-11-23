#include "server_endpoint.h"
#include "kline_server_config.h"

ServerEndpoint::ServerEndpoint()
{

}

ServerEndpoint::~ServerEndpoint()
{

}

void ServerEndpoint::start()
{

}

void ServerEndpoint::init()
{
    std::string server_address(CONFIG->endpoint_addr_);

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

    caller_getklines_ = new GrpcCall<GetKlinesEntity>(call_id, &service_, cq_.get(), provider_);
    callers_[call_id] = caller_getklines_;
    call_id++;

    caller_getlast_ = new GrpcCall<GetLastEntity>(call_id, &service_, cq_.get(), provider_);
    callers_[call_id] = caller_getlast_;
    call_id++;
}

void ServerEndpoint::_handle_rpcs() 
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

void ServerEndpoint::on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& kline)
{

}
