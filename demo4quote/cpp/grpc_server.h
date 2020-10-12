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

#define PUBLISHER utrade::pandora::Singleton<ServerImpl>::GetInstance()

class ServerImpl final {
public:
    ~ServerImpl() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    void run_in_thread(const string& grpc_addr) {
        thread_loop_ = new std::thread(&ServerImpl::_run, this, grpc_addr);
    }

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);
    bool get_snap(const string& exchange, const string& symbol, SDepthQuote& quote) const;

    void on_mix_snap(const string& symbol, const SMixQuote& quote);

    void register_client(CallDataMultiSubscribeQuote* calldata);
    void unregister_client(CallDataMultiSubscribeQuote* calldata);
private:

    // This can be run in multiple threads if needed.
    void _handle_rpcs() {
        // Spawn a new CallData instance to serve new clients.
        new CallDataGetQuote(&service_, cq_.get(), this);
        new CallDataMultiSubscribeQuote(&service_, cq_.get(), this);
        
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
                } else if( cd->call_type_ ==2 ) {
                    static_cast<CallDataMultiSubscribeQuote*>(tag)->Proceed();
                }
            } else {
                CallData* cd = static_cast<CallData*>(tag);
                if( cd->call_type_ == 1 ) {                
                    static_cast<CallDataGetQuote*>(tag)->Release();
                } else if( cd->call_type_ ==2 ) {
                    static_cast<CallDataMultiSubscribeQuote*>(tag)->Release();
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

    std::unique_ptr<ServerCompletionQueue> cq_;
    StreamEngineService::AsyncService service_;
    std::unique_ptr<Server> server_;

    std::thread*               thread_loop_ = nullptr;

    mutable std::mutex                             mutex_clients_;
    unordered_map<CallDataMultiSubscribeQuote*, bool> clients_;
    

    //
    bool _get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) const {
        auto iter = markets_.find(exchange);
        if( iter == markets_.end() )
            return false;
        const TMarketQuote& marketQuote = iter->second;
        auto iter2 = marketQuote.find(symbol);
        if( iter2 == marketQuote.end() )
            return false;
        quote = iter2->second;
        return true;
    };

    mutable std::mutex                             mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;
};