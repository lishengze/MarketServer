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
    TradeClient(){
    }

    void PutMarketStream(const string& symbol, const SMixQuote& quote) {
        if( context_ == nullptr ) {
            auto channel = grpc::CreateChannel(CONFIG->grpc_push_addr_, grpc::InsecureChannelCredentials());
            stub_ = Trade::NewStub(channel);
            context_ = new ClientContext();
            // create channel
            stream_ = StreamPtr{ stub_->PutMarketStream(context_, &response_) };
            int state = channel->GetState(true);
            std::cout << "status is " << state << endl;
            if( state != GRPC_CHANNEL_READY) {
                delete context_;
                context_ = nullptr;
                stream_ = nullptr;
                return;
            }
        }

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
        if( stream_ ) {
            if( !stream_->Write(msd) ) {
                delete context_;
                context_ = nullptr;
                stream_ = nullptr;
            }
        }
            //stream->WritesDone();
    }

private:  
    std::unique_ptr<Trade::Stub> stub_ = nullptr;
    StreamPtr stream_ = nullptr;
    ClientContext *context_ = nullptr;
    google::protobuf::Empty response_;
};

class GrpcPublisher {
public:
    GrpcPublisher() {
        thread_run_ = true;
    }

    ~GrpcPublisher() {
        if (fake_thread_) {
            if (fake_thread_->joinable()) {
                fake_thread_->join();
            }
            delete fake_thread_;        
        }
    }
    void init() {
        //client_ = new TradeClient(grpc::CreateChannel(CONFIG->grpc_push_addr_, grpc::InsecureChannelCredentials()));
    }

    void publish_fake() {
        fake_thread_ = new std::thread(&GrpcPublisher::_test_publish, this);
    }

    void publish(const string& symbol, const SMixQuote& quote) {
        client_.PutMarketStream(symbol, quote);
    }
private: 

    void _test_publish() {
        
        while( thread_run_ ) {
            SMixQuote quote;
            quote.Asks = new SMixDepthPrice();
            quote.Asks->Price.Value = 999;
            quote.Asks->Price.Base = 2;
            quote.Asks->Volume["test"] = 10;
            quote.Bids = new SMixDepthPrice();
            quote.Bids->Price.Value = 900;
            quote.Bids->Price.Base = 2;
            quote.Bids->Volume["test"] = 100;
            
            publish("BTC_USDT", quote);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }

    TradeClient client_;

    // produce fake test msg
    std::thread* fake_thread_ = nullptr;
    std::atomic<bool>          thread_run_;
};