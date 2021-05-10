#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "pandora/util/singleton.hpp"
#include "datacenter.h"
#include "grpc_entity.h"

class ServerEndpoint : public IQuotePusher {
public:
    ~ServerEndpoint() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void init(const string& grpc_addr);

    void start() 
    {
        thread_loop_ = new std::thread(&ServerEndpoint::_handle_rpcs, this);
    }

    void set_cacher(IDataCacher* cacher) { cacher_ = cacher; }

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
    IDataCacher* cacher_ = nullptr;

    // 不做价位交叉处理
    GrpcCall<MarketStream4BrokerEntity>* caller_marketstream4broker_;
    // 做价位交叉处理
    GrpcCall<MarketStream4HedgeEntity>* caller_marketstream4hedge_;
    // 只需要各档总挂单量
    GrpcCall<MarketStream4ClientEntity>* caller_marketstream4client_;
    // otc查询
    GrpcCall<OtcQuoteEntity>* caller_otcquete_;
    // 查询内部参数
    GrpcCall<GetParamsEntity>* caller_getparams_;

    GrpcCall<TradeOrderEntity>* caller_hedge_order_;
};