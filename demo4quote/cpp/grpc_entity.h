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
    GetKlinesEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    GetKlinesEntity* spawn() {
        return new GetKlinesEntity(service_, provider_);
    }

private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    GetKlinesRequest request_;
    GetKlinesResponse reply_;
    ServerAsyncResponseWriter<GetKlinesResponse> responder_;

    IDataProvider* provider_;
};

struct WrapperKlineData: KlineData
{
    char exchange[16];
    char symbol[16];
    int resolution;
};

class GetLastEntity : public BaseGrpcEntity
{
public:
    GetLastEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    void add_data(const WrapperKlineData& kline) {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        datas_.push_back(kline);
        if( total_.size() == 0 || total_.back().index < kline.index ) {
            total_.push_back(kline);
        } else if( total_.back().index == kline.index ) {
            total_.back() = kline;
        }
    }

    GetLastEntity* spawn() {
        return new GetLastEntity(service_, provider_);
    }
private:
    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    GetKlinesRequest request_;
    ServerAsyncWriter<GetKlinesResponse> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    list<WrapperKlineData> datas_;
    list<WrapperKlineData> total_;
    
    IDataProvider* provider_;
};
