#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "risk_controller_define.h"
#include "api.grpc.pb.h"

using grpc::Alarm;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using trade::service::v1::MultiMarketStreamData;
using trade::service::v1::MarketStreamData;
using trade::service::v1::Depth;
using trade::service::v1::DepthData;
using trade::service::v1::EmptyReply;
using trade::service::v1::Trade;


class ServerImpl;
class CallData {
public:
    CallData(ServerCompletionQueue* cq, ServerImpl* parent) : cq_(cq), status_(CREATE), parent_(parent) {}
public:
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_ = nullptr;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, PUSH_TO_BACK, FINISH };
    CallStatus status_;  // The current serving state.

    int call_type_;

    ServerImpl* parent_ = nullptr;
};


class CallDataServeMarketStream : public CallData{
public:
    CallDataServeMarketStream(Trade::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* parent)
        : CallData(cq, parent), service_(service), responder_(&ctx_), times_(0) {
        call_type_ = 1;
        Proceed();
    }

    void Release();

    void Proceed();

    void add_data(std::shared_ptr<MarketStreamData> pdata) {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        datas_.push_back(pdata);
    }

private:
    Trade::AsyncService* service_;
    google::protobuf::Empty request_;
    ServerAsyncWriter<MultiMarketStreamData> responder_;
    int times_;
    grpc::Alarm alarm_;

    mutable std::mutex                        mutex_datas_;
    vector<std::shared_ptr<MarketStreamData>> datas_;
};