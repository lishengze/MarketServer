#pragma once
#include "base/cpp/grpc_call.h"
//#include "grpc_call.h"
#include "risk_controller.grpc.pb.h"
#include "base/cpp/decimal.h"

using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using quote::service::v1::MultiMarketStreamData;
using quote::service::v1::MarketStreamData;
using quote::service::v1::Depth;
using quote::service::v1::MultiMarketStreamDataWithDecimal;
using quote::service::v1::MarketStreamDataWithDecimal;
using quote::service::v1::DepthWithDecimal;
using quote::service::v1::Decimal;
using quote::service::v1::QuoteRequest;
using quote::service::v1::QuoteResponse;
using GrpcRiskControllerService = quote::service::v1::RiskController;
using namespace quote::service::v1;

class IDataCacher;

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

class MarketStream4BrokerEntity : public BaseGrpcEntity
{
public:
    MarketStream4BrokerEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    MarketStream4BrokerEntity* spawn() {
        return new MarketStream4BrokerEntity(service_, cacher_);
    }

    void add_data(std::shared_ptr<void> snap);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;
    
    IDataCacher* cacher_;
    bool snap_sended_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

class MarketStream4HedgeEntity : public BaseGrpcEntity
{
public:
    MarketStream4HedgeEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    MarketStream4HedgeEntity* spawn() {
        return new MarketStream4HedgeEntity(service_, cacher_);
    }

    void add_data(std::shared_ptr<void> snap);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

    IDataCacher* cacher_;
    bool snap_sended_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

class MarketStream4ClientEntity : public BaseGrpcEntity
{
public:
    MarketStream4ClientEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    MarketStream4ClientEntity* spawn() {
        return new MarketStream4ClientEntity(service_, cacher_);
    }

    void add_data(std::shared_ptr<void> snap);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamDataWithDecimal> responder_;

    IDataCacher* cacher_;
    bool snap_sended_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

class OtcQuoteEntity : public BaseGrpcEntity
{
public:
    OtcQuoteEntity(void* service, IDataCacher* cacher);

    void register_call();

    bool process();

    OtcQuoteEntity* spawn() {
        return new OtcQuoteEntity(service_, cacher_);
    }

    //void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcRiskControllerService::AsyncService* service_;
    ServerContext ctx_;

    QuoteRequest request_;
    QuoteResponse reply_;
    ServerAsyncResponseWriter<QuoteResponse> responder_;

    IDataCacher* cacher_;
};