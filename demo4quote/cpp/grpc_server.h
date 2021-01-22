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
#include "quote_mixer2.h"
#include "grpc_entity.h"

class ServerEndpoint : public IKlinePusher, public IMixerQuotePusher
{
public:
    ServerEndpoint(){}
    ~ServerEndpoint() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void init(const string& grpc_addr);

    void start() {
        thread_loop_ = new std::thread(&ServerEndpoint::_handle_rpcs, this);
    }

    void set_cacher(IKlineCacher* cacher) { cacher_ = cacher; }
    void set_quote_cacher(IQuoteCacher* cacher) { quote_cacher_ = cacher; }
    void set_mixer_cacher(IMixerCacher* cacher) { mixer_cacher_ = cacher; }

    // IKlinePusher
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines);

    // IMixerQuotePusher
    void publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update);
    void publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update);
    void publish_trade(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<TradeWithDecimal> trade);

private:
    void _handle_rpcs();

    IKlineCacher* cacher_ = nullptr;
    IQuoteCacher* quote_cacher_ = nullptr;
    IMixerCacher* mixer_cacher_ = nullptr;
    
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
    GrpcCall<SubscribeTradeEntity>* caller_subscribe_trade_;
    GrpcCall<GetLastTradesEntity>* caller_getlast_trades_;
};