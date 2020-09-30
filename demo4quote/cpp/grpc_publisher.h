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
//#include "helper.h"
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

class TradeClient {
public:
    using StreamPtr = boost::shared_ptr<ClientWriter<MarketStreamData>>;
public:
    TradeClient(std::shared_ptr<Channel> channel)
        : stub_(Trade::NewStub(channel)) { 
        stream_ = StreamPtr{ stub_->PutMarketStream(&context_, &response_) };
    }

    void PutMarketStream(const string& symbol, const SMixQuote& quote) {   
   
        MarketStreamData msd; 
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
        stream_->Write(msd);
        //stream->WritesDone();
    }

private:  
    ClientContext context_;
    google::protobuf::Empty response_;
    std::unique_ptr<Trade::Stub> stub_;
    StreamPtr stream_;
};

class GrpcPublisher {
public:
    void init() {
        client_ = new TradeClient(grpc::CreateChannel(CONFIG->grpc_push_addr_, grpc::InsecureChannelCredentials()));
    }

    void publish(const string& symbol, const SMixQuote& quote) {
        client_->PutMarketStream(symbol, quote);
    }
private: 
    TradeClient *client_;
};