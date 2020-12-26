#pragma once

#include "base/cpp/grpc_call.h"
//#include "grpc_call.h"
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

struct SnapAndUpdate{
    std::shared_ptr<void> snap;
    std::shared_ptr<void> update;
};

class GrpcDemoEntity : public BaseGrpcEntity
{
public:
    GrpcDemoEntity(void* service);

    void register_call();

    bool process();

    void add_data(SnapAndUpdate data){}

    GrpcDemoEntity* spawn() {
        return new GrpcDemoEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    DemoReq request_;
    ServerAsyncWriter<DemoResp> responder_;
    
    int times_;
};

//////////////////////////////////////////////////
class SubscribeSingleQuoteEntity : public BaseGrpcEntity
{
public:
    SubscribeSingleQuoteEntity(void* service);

    void register_call();

    bool process();

    void add_data(SnapAndUpdate data);

    SubscribeSingleQuoteEntity* spawn() {
        return new SubscribeSingleQuoteEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    SubscribeQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamDataWithDecimal> responder_;

    // 
    mutable std::mutex                 mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
    bool snap_sended_;
    type_seqno last_seqno;
};

//////////////////////////////////////////////////
class SubscribeMixQuoteEntity : public BaseGrpcEntity
{
public:
    SubscribeMixQuoteEntity(void* service);

    void register_call();

    bool process();

    void add_data(SnapAndUpdate data);

    SubscribeMixQuoteEntity* spawn() {
        return new SubscribeMixQuoteEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    SubscribeMixQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamDataWithDecimal> responder_;

    // 
    mutable std::mutex                 mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
    bool snap_sended_;
    type_seqno last_seqno;
};

//////////////////////////////////////////////////
class SetParamsEntity : public BaseGrpcEntity
{
public:
    SetParamsEntity(void* service);

    void register_call();

    bool process();

    SetParamsEntity* spawn() {
        return new SetParamsEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    SetParamsReq request_;
    SetParamsResp reply_;
    ServerAsyncResponseWriter<SetParamsResp> responder_;
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
    ServerContext ctx_;

    GetParamsReq request_;
    GetParamsResp reply_;
    ServerAsyncResponseWriter<GetParamsResp> responder_;
};

class GetKlinesEntity : public BaseGrpcEntity
{
public:
    GetKlinesEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    GetKlinesEntity* spawn() {
        return new GetKlinesEntity(service_, cacher_);
    }

private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    GetKlinesRequest request_;
    GetKlinesResponse reply_;
    ServerAsyncResponseWriter<GetKlinesResponse> responder_;

    IDataCacher* cacher_;
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
    GetLastEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    void add_data(const WrapperKlineData& data) {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        datas_.push_back(data);
    }

    GetLastEntity* spawn() {
        return new GetLastEntity(service_, cacher_);
    }

private:
    bool _fill_data(MultiGetKlinesResponse& reply);

    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    GetKlinesRequest request_;
    ServerAsyncWriter<MultiGetKlinesResponse> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    list<WrapperKlineData> datas_;
    
    IDataCacher* cacher_;
    
    // 首次发送的缓存
    bool _snap_sended() const { return snap_cached_ && cache_min1_.size() == 0 && cache_min60_.size() == 0; }
    bool snap_cached_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min1_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min60_;
};

//////////////////////////////////////////////////
class SubscribeTradeEntity : public BaseGrpcEntity
{
public:
    SubscribeTradeEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<TradeWithDecimal> data);

    SubscribeTradeEntity* spawn() {
        return new SubscribeTradeEntity(service_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    SubscribeTradeReq request_;
    ServerAsyncWriter<MultiTradeWithDecimal> responder_;

    // 
    mutable std::mutex                 mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};
