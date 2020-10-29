#pragma once

#include "grpc_call.h"
#include "stream_engine_define.h"
#include "stream_engine_server.grpc.pb.h"
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using trade::service::v1::StreamEngineService;
using trade::service::v1::GetQuoteReq;
using trade::service::v1::SubscribeQuoteReq;
using trade::service::v1::SetParamsResp;
using trade::service::v1::SetParamsReq;
using trade::service::v1::MultiQuoteData;
using trade::service::v1::QuoteData;
using trade::service::v1::DepthLevel;
using trade::service::v1::Decimal;
using trade::service::v1::DepthVolume;
using trade::service::v1::DemoReq;
using trade::service::v1::DemoResp;

class GrpcDemoEntity : public BaseGrpcEntity
{
public:
    GrpcDemoEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

private:
    StreamEngineService::AsyncService* service_;

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

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    StreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    GetQuoteReq request_;
    ServerAsyncWriter<MultiQuoteData> responder_;

    // 
    mutable std::mutex                 mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

//////////////////////////////////////////////////
class SubscribeMixQuoteEntity : public BaseGrpcEntity
{
public:
    SubscribeMixQuoteEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    StreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    SubscribeQuoteReq request_;
    ServerAsyncWriter<MultiQuoteData> responder_;

    // 
    mutable std::mutex                 mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};

//////////////////////////////////////////////////
class SetParamsEntity : public BaseGrpcEntity
{
public:
    SetParamsEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

private:
    StreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    SetParamsReq request_;
    SetParamsResp reply_;
    ServerAsyncResponseWriter<SetParamsResp> responder_;
};

