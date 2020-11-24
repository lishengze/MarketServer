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
    init();
    thread_loop_ = new std::thread(&ServerEndpoint::_handle_rpcs, this);
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
        caller_getlast_->add_data(v);
    }
}
