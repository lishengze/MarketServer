#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "risk_controller_define.h"
#include "api.grpc.pb.h"
#include "grpc_caller.h"
#include "pandora/util/singleton.hpp"
#include "datacenter.h"

using grpc::Alarm;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using trade::service::v1::MarketStreamData;
using trade::service::v1::Depth;
using trade::service::v1::DepthData;
using trade::service::v1::EmptyReply;
using trade::service::v1::Trade;

#define PUBLISHER utrade::pandora::Singleton<ServerImpl>::GetInstance()

class ServerImpl final {
public:
    ~ServerImpl() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    void run_in_thread(const string& grpc_addr, DataCenter* dc) {
        dc_ = dc;
        thread_loop_ = new std::thread(&ServerImpl::_run, this, grpc_addr);
    }

    void register_client(CallDataServeMarketStream* calldata);
    void unregister_client(CallDataServeMarketStream* calldata);
private:

    // This can be run in multiple threads if needed.
    void _handle_rpcs() {
        // Spawn a new CallData instance to serve new clients.
        new CallDataServeMarketStream(&service_, cq_.get(), this);
        
        void* tag;  // uniquely identifies a request.
        bool ok;
        while (true) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallData instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq_->Next(&tag, &ok));
            //GPR_ASSERT(ok);
            if( ok ) {
                CallData* cd = static_cast<CallData*>(tag);
                if( cd->call_type_ == 1 ) {                
                    static_cast<CallDataServeMarketStream*>(tag)->Proceed();
                }
            } else {
                CallData* cd = static_cast<CallData*>(tag);
                if( cd->call_type_ == 1 ) {                
                    static_cast<CallDataServeMarketStream*>(tag)->Release();
                }
            }
        }
    }

    // There is no shutdown handling in this code.
    void _run(const string& grpc_addr) {
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
        _handle_rpcs();
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    Trade::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread*               thread_loop_ = nullptr;

    DataCenter* dc_;
};