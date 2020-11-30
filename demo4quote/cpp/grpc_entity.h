#pragma once

#include "grpc_call.h"
#include "stream_engine_define.h"
#include "stream_engine.grpc.pb.h"
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
using quote::service::v1::MultiMarketStreamData;
using quote::service::v1::MarketStreamData;
using quote::service::v1::Depth;
using quote::service::v1::SubscribeMixQuoteReq;

class GrpcDemoEntity : public BaseGrpcEntity
{
public:
    GrpcDemoEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

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

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcStreamEngineService::AsyncService* service_;

    ServerContext ctx_;

    SubscribeQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

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

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    SubscribeMixQuoteReq request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

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

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

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

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update){}

private:
    GrpcStreamEngineService::AsyncService* service_;
    ServerContext ctx_;

    GetParamsReq request_;
    GetParamsResp reply_;
    ServerAsyncResponseWriter<GetParamsResp> responder_;
};

