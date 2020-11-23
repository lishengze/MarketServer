#pragma once
#include "kline_mixer.h"
#include "base/cpp/grpc_call.h"
#include "kline_server.grpc.pb.h"

using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using quote::service::v1::GetKlinesResponse;
using quote::service::v1::GetKlinesRequest;
using GrpcKlineService = quote::service::v1::KlineServer;

class GetKlinesEntity : public BaseGrpcEntity
{
public:
    GetKlinesEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

private:
    GrpcKlineService::AsyncService* service_;
    ServerContext ctx_;

    GetKlinesRequest request_;
    GetKlinesResponse reply_;
    ServerAsyncResponseWriter<GetKlinesResponse> responder_;
};

class GetLastEntity : public BaseGrpcEntity
{
public:
    GetLastEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcKlineService::AsyncService* service_;

    ServerContext ctx_;

    GetKlinesRequest request_;
    ServerAsyncWriter<GetKlinesResponse> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};