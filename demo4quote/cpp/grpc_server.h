#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "pandora/util/singleton.hpp"
#include "grpc_entity.h"

#define PUBLISHER utrade::pandora::Singleton<GrpcServer>::GetInstance()

class GrpcServer final {
public:
    ~GrpcServer() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void run_in_thread(const string& grpc_addr) {
        thread_loop_ = new std::thread(&GrpcServer::_run, this, grpc_addr);
    }

    void publish_single(const string& exchange, const string& symbol, std::shared_ptr<QuoteData> snap, std::shared_ptr<QuoteData> update);
    void publish_mix(const string& symbol, std::shared_ptr<QuoteData> snap, std::shared_ptr<QuoteData> update);
   
private:

    void _handle_rpcs();

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

    // grpc对象
    std::unique_ptr<ServerCompletionQueue> cq_;
    StreamEngineService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread* thread_loop_ = nullptr;

    CommonGrpcCall* caller_demo_;
    CommonGrpcCall* caller_subscribe_single_;
    CommonGrpcCall* caller_subscribe_mix_;
    CommonGrpcCall* caller_setparams_;
};