#pragma once
#include "grpc_call.h"
#include "risk_controller.grpc.pb.h"

using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using quote::service::v1::MultiMarketStreamData;
using quote::service::v1::MarketStreamData;
using quote::service::v1::Depth;
using GrpcRiskControllerService = quote::service::v1::RiskController;


class MarketStream4BrokerEntity : public BaseGrpcEntity
{
public:
    MarketStream4BrokerEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

class MarketStream4HedgeEntity : public BaseGrpcEntity
{
public:
    MarketStream4HedgeEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

class MarketStream4ClientEntity : public BaseGrpcEntity
{
public:
    MarketStream4ClientEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcRiskControllerService::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};
