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

    void init(const string& grpc_addr);

    void run_in_thread() {
        thread_loop_ = new std::thread(&GrpcServer::_handle_rpcs, this);
    }

    void publish4Broker(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
    void publish4Hedge(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
    void publish4Client(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
private:

    void _handle_rpcs();
    
    // grpc对象
    std::unique_ptr<ServerCompletionQueue> cq_;
    GrpcRiskControllerService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread* thread_loop_ = nullptr;

    // 不做价位交叉处理
    CommonGrpcCall* caller_marketstream4broker_;
    // 做价位交叉处理
    CommonGrpcCall* caller_marketstream4hedge_;
    // 只需要各档总挂单量
    CommonGrpcCall* caller_marketstream4client_;
    unordered_map<int, CommonGrpcCall*> callers_;
};