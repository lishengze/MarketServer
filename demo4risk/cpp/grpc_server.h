#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "grpc_entity.h"
#include "pandora/util/singleton.hpp"

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

    void publish(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
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

    std::unique_ptr<ServerCompletionQueue> cq_;
    Broker::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread*               thread_loop_ = nullptr;

    CommonGrpcCall* caller_marketstream_;
};