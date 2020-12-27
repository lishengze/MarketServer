#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include <google/protobuf/empty.pb.h>
#include "stream_engine.grpc.pb.h"
#include "hub_struct.h"
#include "updater_quote.h"
#include "base/cpp/quote.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::StreamEngine;
using quote::service::v1::SubscribeTradeReq;
using SETrade = quote::service::v1::TradeWithDecimal;
using quote::service::v1::MultiTradeWithDecimal;
using quote::service::v1::GetLatestTradesReq;
using quote::service::v1::GetLatestTradesResp;


class ITradeUpdater {
public:
    virtual void on_trade(const SETrade& quote) = 0;
};

inline void RequestLastTrades(const string& addr, vector<SETrade>& trades)
{
    trades.clear();

    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);
    
    GetLatestTradesReq req;
    GetLatestTradesResp resp;
    ClientContext context;

    stub->GetLatestTrades(&context, req, &resp);

    for( int i = 0 ; i < resp.trades_size() ; i ++ ) {
        const SETrade& trade = resp.trades(i);
        trades.push_back(trade);
    }
}

class TradeUpdater 
{
public:
    TradeUpdater(){}
    ~TradeUpdater(){}

    void start(const string& addr, ITradeUpdater* callback) {
        thread_loop_ = new std::thread(&TradeUpdater::_run, this, addr, callback);
    }

private:

    void _request(const string& addr, ITradeUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

        SubscribeTradeReq req;
        MultiTradeWithDecimal resp;
        ClientContext context;

        std::unique_ptr<ClientReader<MultiTradeWithDecimal> > reader(stub->SubscribeTrade(&context, req));
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
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                std::cout << "status is GRPC_CHANNEL_SHUTDOWN" << endl;
                break;
            }
        }
        while (reader->Read(&resp)) {
            // split and convert
            std::cout << "get " << resp.trades_size() << " items" << std::endl;
            for( int i = 0 ; i < resp.trades_size() ; i ++ )
            {
                const SETrade& trade = resp.trades(i);
                callback->on_trade(trade);
            }
        }
        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "GetLast SubscribeTrade succeeded." << std::endl;
        } else {
            std::cout << "GetLast SubscribeTrade failed." << std::endl;
        }
    }

    void _run(const string& addr, ITradeUpdater* callback) {
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};