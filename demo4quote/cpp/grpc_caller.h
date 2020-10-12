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

#include "stream_engine_define.h"
#include "stream_engine_server.grpc.pb.h"

using grpc::Alarm;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using trade::service::v1::StreamEngineService;
using trade::service::v1::GetQuoteReq;
using trade::service::v1::SubscribeQuoteReq;
using trade::service::v1::MultiQuoteData;
using trade::service::v1::QuoteData;
using trade::service::v1::DepthLevel;
using trade::service::v1::Decimal;
using trade::service::v1::DepthVolume;


class ServerImpl;
class CallData {
public:
    CallData(ServerCompletionQueue* cq, ServerImpl* parent) : parent_(parent), cq_(cq), status_(CREATE) {}
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

// Class encompasing the state and logic needed to serve a request.
class CallDataGetQuote : public CallData{
public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallDataGetQuote(StreamEngineService::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* parent)
        : CallData(cq, parent), service_(service), responder_(&ctx_) {
        call_type_ = 1;
        // Invoke the serving logic right away.
        Proceed();
    }

    void Release();

    void Proceed();

private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    StreamEngineService::AsyncService* service_;

    // What we get from the client.
    GetQuoteReq request_;
    // What we send back to the client.

    // The means to get back to the client.
    ServerAsyncResponseWriter<QuoteData> responder_;
};

class CallDataMultiSubscribeQuote : public CallData{
public:
    CallDataMultiSubscribeQuote(StreamEngineService::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* parent)
        : CallData(cq, parent), service_(service), responder_(&ctx_), times_(0) {
        call_type_ = 2;
        Proceed();
    }

    void Release();

    void Proceed();

    void add_data(std::shared_ptr<QuoteData> pdata) {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        datas_.push_back(pdata);
    }

private:
    StreamEngineService::AsyncService* service_;
    SubscribeQuoteReq request_;
    ServerAsyncWriter<MultiQuoteData> responder_;
    int times_;
    grpc::Alarm alarm_;

    mutable std::mutex                             mutex_datas_;
    vector<std::shared_ptr<QuoteData>> datas_;
};