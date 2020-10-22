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
#include "grpc_caller.h"
#include "pandora/util/singleton.hpp"

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

#define PUBLISHER utrade::pandora::Singleton<GrpcServer>::GetInstance()

class GrpcServer final {
public:
    ~GrpcServer() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    void run_in_thread(const string& grpc_addr) {
        thread_loop_ = new std::thread(&GrpcServer::_run, this, grpc_addr);
    }

    void on_snap(const string& exchange, const string& symbol, std::shared_ptr<QuoteData> quote);
    void on_mix_snap(const string& symbol, std::shared_ptr<QuoteData> quote);

    void register_client(CallDataMultiSubscribeQuote* calldata);
    void unregister_client(CallDataMultiSubscribeQuote* calldata);
    void register_client2(CallDataSubscribeOneQuote* calldata);
    void unregister_client2(CallDataSubscribeOneQuote* calldata);
private:

    // This can be run in multiple threads if needed.
    void _handle_rpcs() {
        // Spawn a new CallData instance to serve new clients.
        new CallDataGetQuote(&service_, cq_.get(), this);
        new CallDataMultiSubscribeQuote(&service_, cq_.get(), this);
        new CallDataSubscribeOneQuote(&service_, cq_.get(), this);
        new CallDataSetParams(&service_, cq_.get(), this);
        
        void* tag;  // uniquely identifies a request.
        bool ok;
        while (true) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallData instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq_->Next(&tag, &ok));
            //GPR_ASSERT(ok);
            if( ok ) {
                CallData* cd = static_cast<CallData*>(tag);
                if( cd->call_type_ == 1 ) {                
                    static_cast<CallDataGetQuote*>(tag)->Proceed();
                } else if( cd->call_type_ == 2 ) {
                    static_cast<CallDataMultiSubscribeQuote*>(tag)->Proceed();
                } else if( cd->call_type_ == 3 ) {
                    static_cast<CallDataSubscribeOneQuote*>(tag)->Proceed();
                } else if( cd->call_type_ == 4 ) {
                    static_cast<CallDataSetParams*>(tag)->Proceed();
                }
            } else {
                CallData* cd = static_cast<CallData*>(tag);
                if( cd->call_type_ == 1 ) {                
                    static_cast<CallDataGetQuote*>(tag)->Release();
                } else if( cd->call_type_ == 2 ) {
                    static_cast<CallDataMultiSubscribeQuote*>(tag)->Release();
                } else if( cd->call_type_ == 3 ) {
                    static_cast<CallDataSubscribeOneQuote*>(tag)->Release();
                } else if( cd->call_type_ == 4 ) {
                    static_cast<CallDataSetParams*>(tag)->Release();
                }
            }
        }
    }

    // There is no shutdown handling in this code.
    void _run(const string& grpc_addr) {
        std::string server_address(grpc_addr);

        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // Register "service_" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *asynchronous* service.
        builder.RegisterService(&service_);
        // Get hold of the completion queue used for the asynchronous communication
        // with the gRPC runtime.
        cq_ = builder.AddCompletionQueue();
        // Finally assemble the server.
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // Proceed to the server's main loop.
        _handle_rpcs();
    }

    // grpc对象
    std::unique_ptr<ServerCompletionQueue> cq_;
    StreamEngineService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread*               thread_loop_ = nullptr;

    // 聚合行情推送客户端
    mutable std::mutex                             mutex_clients_;
    unordered_map<CallDataMultiSubscribeQuote*, bool> clients_;

    // 单交易所单品种推送kehduuan
    mutable std::mutex                             mutex_clients2_;
    unordered_map<CallDataSubscribeOneQuote*, bool> clients2_;
};