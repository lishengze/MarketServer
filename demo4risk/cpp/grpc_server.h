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

#define PUBLISHER utrade::pandora::Singleton<ServerEndpoint>::GetInstance()

class ServerEndpoint final {
public:
    ~ServerEndpoint() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void init(const string& grpc_addr);

    void run_in_thread() {
        thread_loop_ = new std::thread(&ServerEndpoint::_handle_rpcs, this);
    }

    // 发布聚合行情（用于撮合）
    // 发布风控处理后的行情
    void publish4Broker(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);

    // 发布聚合行情（用于对冲）
    // 发送风控处理前的行情
    void publish4Hedge(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);

    // 发布聚合行情（用于客户端显示）
    // 发布风控处理后的行情
    // 使用精确价格和量，只需要total
    void publish4Client(const string& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update);
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