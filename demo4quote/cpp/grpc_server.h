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
    GrpcServer(){}
    ~GrpcServer() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void init(const string& grpc_addr);

    void run_in_thread() {
        thread_loop_ = new std::thread(&GrpcServer::_handle_rpcs, this);
    }

    void publish_single(const string& exchange, const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
    void publish_mix(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
   
private:
    void _handle_rpcs();
    
    // grpc对象
    std::unique_ptr<ServerCompletionQueue> cq_;
    GrpcStreamEngineService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread* thread_loop_ = nullptr;

    CommonGrpcCall* caller_demo_;
    CommonGrpcCall* caller_subscribe_single_;
    CommonGrpcCall* caller_subscribe_mix_;
    CommonGrpcCall* caller_setparams_;
    CommonGrpcCall* caller_getparams_;
    unordered_map<int, CommonGrpcCall*> callers_;
};