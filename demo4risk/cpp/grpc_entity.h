#pragma once
#include "grpc_call.h"
#include "api.grpc.pb.h"

using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using broker::service::v1::MultiMarketStreamData;
using broker::service::v1::MarketStreamData;
using broker::service::v1::Depth;
using broker::service::v1::EmptyReply;
using broker::service::v1::Broker;


class MarketStreamEntity : public BaseGrpcEntity
{
public:
    MarketStreamEntity(void* service);

    void register_call();

    bool process();

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update);

private:
    Broker::AsyncService* service_;

    ServerContext ctx_;

    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    vector<std::shared_ptr<void>> datas_;
};