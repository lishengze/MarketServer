#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#ifdef BAZEL_BUILD
#include "api.grpc.pb.h"
#else
#include "api.grpc.pb.h"
#endif
#include "stream_engine_define.h"
#include "stream_engine_config.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using trade::service::v1::MarketStreamData;
using trade::service::v1::Depth;
using trade::service::v1::DepthData;
using trade::service::v1::EmptyReply;
using trade::service::v1::Trade;

inline void make_market_stream(const string& symbol, const SMixQuote& quote, MarketStreamData& msd) {
    msd.set_symbol(symbol);
    // 卖盘
    {
        SMixDepthPrice* ptr = quote.Asks;
        while( ptr != NULL ) {
            Depth* depth = msd.add_ask_depth();
            depth->set_price(ptr->Price.GetValue());
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthData* depthData = depth->add_data();
                depthData->set_size(volume);
                depthData->set_exchange(exchange);
            }
            ptr = ptr->Next;
        }
    }
    // 买盘
    {
        SMixDepthPrice* ptr = quote.Bids;
        while( ptr != NULL ) {
            Depth* depth = msd.add_bid_depth();
            depth->set_price(ptr->Price.GetValue());
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthData* depthData = depth->add_data();
                depthData->set_size(volume);
                depthData->set_exchange(exchange);
            }
            ptr = ptr->Next;
        }
    }
};

class GrpcClient {
public:
public:
    GrpcClient(const string& addr) : addr_(addr){
    }

    void PutMarketStream(const string& symbol, const SMixQuote& quote) {
        if( context_ == nullptr && !init_context() ) {
            return;
        }

        MarketStreamData msd; 
        make_market_stream(symbol, quote, msd);

        if( context_ && stream_ ) {
            if( !stream_->Write(msd) ) {
                std::cout << "write fail" << endl;
                stub_ = nullptr;
                context_ = nullptr;
                stream_ = nullptr;
            }
        }
    }

private:  

    bool init_context() {
        auto channel = grpc::CreateChannel(addr_, grpc::InsecureChannelCredentials());
        stub_ = Trade::NewStub(channel);
        context_ = std::unique_ptr<ClientContext>(new ClientContext());
        stream_ = std::unique_ptr<ClientWriter<MarketStreamData>>(stub_->PutMarketStream(context_.get(), &response_));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                std::cout << "status is GRPC_CHANNEL_IDLE" << endl;
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                std::cout << "status is GRPC_CHANNEL_CONNECTING" << endl;
                break;
            }
            case GRPC_CHANNEL_READY: {           
                std::cout << "status is GRPC_CHANNEL_READY" << endl;
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                std::cout << "status is GRPC_CHANNEL_TRANSIENT_FAILURE" << endl;
                stub_ = nullptr;
                context_ = nullptr;
                stream_ = nullptr;
                break;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                std::cout << "status is GRPC_CHANNEL_SHUTDOWN" << endl;
                break;
            }
        }
        return true;
    }

    // address
    std::string addr_;

    // for empty responose
    google::protobuf::Empty response_;

    // 
    std::unique_ptr<ClientContext> context_ = nullptr;
    std::unique_ptr<Trade::Stub> stub_ = nullptr;
    std::unique_ptr<ClientWriter<MarketStreamData>> stream_ = nullptr;
};

class GrpcPublisher {
public:
    GrpcPublisher() {
    }

    ~GrpcPublisher() {
    }
    void init() {
        client_ = new GrpcClient(CONFIG->grpc_push_addr_);
    }

    void publish(const string& symbol, const SMixQuote& quote) {
        if( client_ )
            client_->PutMarketStream(symbol, quote);
    }
private: 
    GrpcClient* client_ = nullptr;
};