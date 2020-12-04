#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "pandora/util/singleton.hpp"
#include "kline_mixer.h"
#include "grpc_entity.h"

#define PUBLISHER utrade::pandora::Singleton<ServerEndpoint>::GetInstance()

class ServerEndpoint : public IMixerKlinePusher 
{
public:
    ServerEndpoint(){}
    ~ServerEndpoint() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void init(const string& grpc_addr);

    void run_in_thread() {
        thread_loop_ = new std::thread(&ServerEndpoint::_handle_rpcs, this);
    }

    void publish_single(const string& exchange, const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
    void publish_mix(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update);
    void publish_config(const std::unordered_map<TSymbol, SNacosConfig>& symbols);

    void set_provider(IDataProvider* provider) { provider_ = provider; }
    // IMixerKlinePusher
    void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines);
private:
    void _handle_rpcs();

    IDataProvider* provider_ = nullptr;
    
    // grpc对象
    std::unique_ptr<ServerCompletionQueue> cq_;
    GrpcStreamEngineService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread* thread_loop_ = nullptr;

    GrpcCall<GrpcDemoEntity>* caller_demo_;
    GrpcCall<SubscribeSingleQuoteEntity>* caller_subscribe_single_;
    GrpcCall<SubscribeMixQuoteEntity>* caller_subscribe_mix_;
    GrpcCall<SetParamsEntity>* caller_setparams_;
    GrpcCall<GetParamsEntity>* caller_getparams_;
    GrpcCall<GetKlinesEntity>* caller_getklines_;
    GrpcCall<GetLastEntity>* caller_getlast_;
    unordered_map<int, CommonGrpcCall*> callers_;
};