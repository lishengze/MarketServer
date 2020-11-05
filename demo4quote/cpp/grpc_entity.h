#pragma once

#include "grpc_call.h"
#include "stream_engine_define.h"
#include "stream_engine_server.grpc.pb.h"
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using trade::service::v1::StreamEngineService;
using trade::service::v1::SetParamsResp;
using trade::service::v1::SetParamsReq;
using trade::service::v1::GetParamsResp;
using trade::service::v1::GetParamsReq;
using trade::service::v1::DemoReq;
using trade::service::v1::DemoResp;
using trade::service::v1::MultiSubscribeQuoteReq;
using trade::service::v1::MultiMarketStreamData;
using trade::service::v1::MarketStreamData;
using trade::service::v1::Depth;
using trade::service::v1::SubscribeOneQuoteReq;
using trade::service::v1::MultiQuoteData;
using trade::service::v1::QuoteData;
using trade::service::v1::DepthLevel;
using trade::service::v1::Decimal;

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

    SubscribeOneQuoteReq request_;
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

    MultiSubscribeQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

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

//////////////////////////////////////////////////
class GetParamsEntity : public BaseGrpcEntity
{
public:
    GetParamsEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

private:
    StreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    GetParamsReq request_;
    GetParamsResp reply_;
    ServerAsyncResponseWriter<GetParamsResp> responder_;
};


