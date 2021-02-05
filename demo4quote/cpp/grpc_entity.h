#pragma once

#include "base/cpp/grpc_server.h"
#include "base/cpp/concurrentqueue.h"
#include "base/cpp/decimal.h"
#include "stream_engine_define.h"
#include "stream_engine.grpc.pb.h"
#include "kline_mixer.h"
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using GrpcStreamEngineService = quote::service::v1::StreamEngine;
using quote::service::v1::SetParamsResp;
using quote::service::v1::SetParamsReq;
using quote::service::v1::GetParamsResp;
using quote::service::v1::GetParamsReq;
using quote::service::v1::DemoReq;
using quote::service::v1::DemoResp;
using quote::service::v1::SubscribeQuoteReq;
using quote::service::v1::MultiMarketStreamDataWithDecimal;
using quote::service::v1::MarketStreamDataWithDecimal;
using quote::service::v1::DepthWithDecimal;
using quote::service::v1::Decimal;
using quote::service::v1::SubscribeMixQuoteReq;
using quote::service::v1::GetKlinesResponse;
using quote::service::v1::GetKlinesRequest;
using quote::service::v1::MultiGetKlinesResponse;
using quote::service::v1::Kline;
using quote::service::v1::SubscribeTradeReq;
using quote::service::v1::TradeWithDecimal;
using quote::service::v1::MultiTradeWithDecimal;
using quote::service::v1::GetLatestTradesReq;
using quote::service::v1::GetLatestTradesResp;

class IQuoteCacher;
class IMixerCacher;

using TradePtr = std::shared_ptr<TradeWithDecimal>;
using StreamDataPtr = std::shared_ptr<MarketStreamDataWithDecimal>;

inline void set_decimal(Decimal* dst, const SDecimal& src)
{
    dst->set_base(src.data_.real_.value_);
    dst->set_prec(src.data_.real_.prec_);
}

inline void set_decimal(Decimal* dst, const Decimal& src)
{
    dst->set_base(src.base());
    dst->set_prec(src.prec());
}

//////////////////////////////////////////////////
class SubscribeSingleQuoteEntity : public BaseGrpcEntity
{
public:
    SubscribeSingleQuoteEntity(void* service, IQuoteCacher* cacher);

    void register_call();

    bool process();

    void on_init();

    void add_data(StreamDataPtr data);

    SubscribeSingleQuoteEntity* spawn() {
        return new SubscribeSingleQuoteEntity(service_, cacher_);
    }
private:

    bool _is_filtered(const TExchange& exchange, const TSymbol& symbol);

    GrpcStreamEngineService::AsyncService* service_;
    SubscribeQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamDataWithDecimal> responder_;

    IQuoteCacher* cacher_ = nullptr;

    // 
    moodycamel::ConcurrentQueue<StreamDataPtr> datas_;
    //mutable std::mutex            mutex_datas_;
    //vector<std::shared_ptr<void>> datas_;
};

//////////////////////////////////////////////////
class SubscribeMixQuoteEntity : public BaseGrpcEntity
{
public:
    SubscribeMixQuoteEntity(void* service, IQuoteCacher* cacher);

    void register_call();

    bool process();

    void on_init();

    void add_data(StreamDataPtr data);

    SubscribeMixQuoteEntity* spawn() {
        return new SubscribeMixQuoteEntity(service_, cacher_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;
    SubscribeMixQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamDataWithDecimal> responder_;

    IQuoteCacher* cacher_ = nullptr;

    // 
    moodycamel::ConcurrentQueue<StreamDataPtr> datas_;
    //mutable std::mutex                 mutex_datas_;
    //vector<std::shared_ptr<void>> datas_;
};

//////////////////////////////////////////////////
class GetParamsEntity : public BaseGrpcEntity
{
public:
    GetParamsEntity(void* service);

    void register_call();

    bool process();

    GetParamsEntity* spawn() {
        return new GetParamsEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;
    GetParamsReq request_;
    ServerAsyncResponseWriter<GetParamsResp> responder_;
};

//////////////////////////////////////////////////
class GetKlinesEntity : public BaseGrpcEntity
{
public:
    GetKlinesEntity(void* service, IKlineCacher* cacher);

    void register_call();

    bool process();

    GetKlinesEntity* spawn() {
        return new GetKlinesEntity(service_, cacher_);
    }

private:
    GrpcStreamEngineService::AsyncService* service_;
    GetKlinesRequest request_;
    ServerAsyncResponseWriter<GetKlinesResponse> responder_;

    IKlineCacher* cacher_;
};

struct WrapperKlineData: KlineData
{
    string exchange;
    string symbol;
    int resolution;

    vector<KlineData> klines;
};

class GetLastEntity : public BaseGrpcEntity
{
public:
    GetLastEntity(void* service, IKlineCacher* cacher);

    void register_call();

    void on_init();

    bool process();

    void add_data(const WrapperKlineData& data);

    GetLastEntity* spawn() {
        return new GetLastEntity(service_, cacher_);
    }

private:
    //bool _fill_data(MultiGetKlinesResponse& reply);

    GrpcStreamEngineService::AsyncService* service_;
    GetKlinesRequest request_;
    ServerAsyncWriter<MultiGetKlinesResponse> responder_;
    
    IKlineCacher* cacher_ = nullptr;

    // 
    moodycamel::ConcurrentQueue<WrapperKlineData> datas_;
    //mutable std::mutex            mutex_datas_;
    //list<WrapperKlineData> datas_;
     
    // 首次发送的缓存
    //bool _snap_sended() const { return snap_cached_ && cache_min1_.size() == 0 && cache_min60_.size() == 0; }
    //bool snap_cached_;
    //unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min1_;
    //unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min60_;
};

//////////////////////////////////////////////////
class SubscribeTradeEntity : public BaseGrpcEntity
{
public:
    SubscribeTradeEntity(void* service);

    void register_call();

    bool process();

    void add_data(TradePtr data);

    SubscribeTradeEntity* spawn() {
        return new SubscribeTradeEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;
    SubscribeTradeReq request_;
    ServerAsyncWriter<MultiTradeWithDecimal> responder_;

    // 
    moodycamel::ConcurrentQueue<TradePtr> datas_;
    //mutable std::mutex                 mutex_datas_;
    //vector<std::shared_ptr<void>> datas_;
};

//////////////////////////////////////////////////////////////
class GetLastTradesEntity : public BaseGrpcEntity
{
public:
    GetLastTradesEntity(void* service, IQuoteCacher* cacher);

    void register_call();

    bool process();

    GetLastTradesEntity* spawn() {
        return new GetLastTradesEntity(service_, cacher_);
    }

private:
    bool _fill_data(MultiGetKlinesResponse& reply);

    GrpcStreamEngineService::AsyncService* service_;
    GetLatestTradesReq request_;
    ServerAsyncResponseWriter<GetLatestTradesResp> responder_;
    
    IQuoteCacher* cacher_;    
};
