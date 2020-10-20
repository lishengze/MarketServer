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

inline void make_cache(const MarketStreamData* msd, bool isAsk, std::unordered_map<string, double>& cache) {
    if( isAsk ) {
        for( int i = 0 ; i < msd->ask_depth_size() ; ++i ){
            const Depth& depth = msd->ask_depth(i);
            double volume = 0;
            for( int j = 0 ; j < depth.data_size() ; ++j ){
                const DepthData& depthData = depth.data(j);
                volume += depthData.size();
            }
            cache[depth.price()] = volume;
        }
    } else {
        for( int i = 0 ; i < msd->bid_depth_size() ; ++i ){
            const Depth& depth = msd->bid_depth(i);
            double volume = 0;
            for( int j = 0 ; j < depth.data_size() ; ++j ){
                const DepthData& depthData = depth.data(j);
                volume += depthData.size();
            }
            cache[depth.price()] = volume;
        }
    }
};

inline void make_diff_market_stream(const string& symbol, const MarketStreamData* last, const MarketStreamData* current, MarketStreamData* msd) {
    msd->set_symbol(symbol);
    msd->set_is_cover(false);
    // 卖盘
    {
        std::unordered_map<string, double> lastCache;
        make_cache(last, true, lastCache);
        std::unordered_map<string, double> curCache;
        make_cache(current, true, curCache);
    }
};

inline void make_market_stream(const string& symbol, const SMixQuote& quote, MarketStreamData* msd) {
    msd->set_symbol(symbol);
    msd->set_is_cover(true);
    char sequence[256];
    sprintf(sequence, "%lld", quote.SequenceNo);
    msd->set_msg_seq(sequence);

    // 卖盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = quote.Asks;
        while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
            Depth* depth = msd->add_ask_depth();
            depth->set_price(ptr->Price.GetStrValue());
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthData* depthData = depth->add_data();
                depthData->set_size(volume);
                depthData->set_exchange(exchange);
            }
            depth_count += 1;
            ptr = ptr->Next;
        }
    }
    // 买盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = quote.Bids;
        while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
            Depth* depth = msd->add_bid_depth();
            depth->set_price(ptr->Price.GetStrValue());
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthData* depthData = depth->add_data();
                depthData->set_size(volume);
                depthData->set_exchange(exchange);
            }
            depth_count += 1;
            ptr = ptr->Next;
        }
    }
};

class GrpcClient {
public:
    GrpcClient(const string& addr) : addr_(addr){
    }

    void PutMarketStream(const string& symbol, const SMixQuote& quote, bool isSnap) {
        if( context_ == nullptr && !init_context() ) {
            return;
        }
        
        std::shared_ptr<MarketStreamData> msd(new MarketStreamData()), pushd = msd;
        make_market_stream(symbol, quote, msd.get());
        
        if( !isSnap ) {
            auto iter = cache_.find(symbol);
            if( iter != cache_.end() ) {
                //pushd = std::shared_ptr<MarketStreamData>(new MarketStreamData());
                //make_diff_market_stream(symbol, iter->second.get(), msd.get(), pushd.get());
            }
        }
        cache_[symbol] = msd;

        if( context_ && stream_ ) {
            if( !stream_->Write(*pushd.get()) ) {
                std::cout << "write fail" << endl;
                release_context();
            }
        }
    }

private:  
    void release_context() {
        stub_ = nullptr;
        context_ = nullptr;
        stream_ = nullptr;
        cache_.clear();
    }

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
                release_context();
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

    // push cache
    std::unordered_map<string, std::shared_ptr<MarketStreamData>> cache_;
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

    void publish(const string& symbol, const SMixQuote& quote, bool isSnap) {
        if( client_ )
            client_->PutMarketStream(symbol, quote, isSnap);
    }
private: 
    GrpcClient* client_ = nullptr;
};