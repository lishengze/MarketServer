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
#include "update_quote.h"
#include "base/cpp/quote.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::StreamEngine;
using quote::service::v1::SubscribeTradeReq;
using quote::service::v1::SubscribeQuoteReq;
using SETrade = quote::service::v1::TradeWithDecimal;
using quote::service::v1::MultiTradeWithDecimal;
using quote::service::v1::GetLatestTradesReq;
using quote::service::v1::GetLatestTradesResp;
using quote::service::v1::DataInBinary;


class IStreamEngineUpdater {
public:
    virtual void on_trade(const SETrade& quote) = 0;
    virtual void on_raw_depth(const SEData& quote) = 0;
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

// 从StreamEngine订阅成交数据
/*
class TradeUpdater 
{
public:
    TradeUpdater(){}
    ~TradeUpdater(){}

    void start(const string& addr, IStreamEngineUpdater* callback) {
        thread_loop_ = new std::thread(&TradeUpdater::_run, this, addr, callback);
    }

private:

    void _request(const string& addr, IStreamEngineUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

        SubscribeTradeReq req;
        MultiTradeWithDecimal resp;
        ClientContext context;

        std::unique_ptr<ClientReader<MultiTradeWithDecimal> > reader(stub->SubscribeTrade(&context, req));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                std::cout << "[TradeUpdater] status is GRPC_CHANNEL_IDLE" << endl;
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                std::cout << "[TradeUpdater] status is GRPC_CHANNEL_CONNECTING" << endl;
                break;
            }
            case GRPC_CHANNEL_READY: {           
                std::cout << "[TradeUpdater] status is GRPC_CHANNEL_READY" << endl;
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                std::cout << "[TradeUpdater] status is GRPC_CHANNEL_TRANSIENT_FAILURE" << endl;
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                std::cout << "[TradeUpdater] status is GRPC_CHANNEL_SHUTDOWN" << endl;
                break;
            }
        }
        while (reader->Read(&resp)) {
            // split and convert
            //std::cout << "get " << resp.trades_size() << " items" << std::endl;
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

    void _run(const string& addr, IStreamEngineUpdater* callback) {
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};
*/
// 从StreamEngine订阅行情数据
class DepthUpdater 
{
public:

    void start(const string& addr, IStreamEngineUpdater* callback) {
        thread_loop_ = new std::thread(&DepthUpdater::_run, this, addr, callback);
    }

private:

    void _request(const string& addr, IStreamEngineUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

        SubscribeQuoteReq req;
        SEMultiData multiQuote;
        DataInBinary dataInBinary;
        ClientContext context;

        //std::unique_ptr<ClientReader<SEMultiData> > reader(stub->SubscribeQuote(&context, req));
        std::unique_ptr<ClientReader<DataInBinary> > reader(stub->SubscribeQuoteInBinary(&context, req));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                std::cout << "[DepthUpdater] status is GRPC_CHANNEL_IDLE" << endl;
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                std::cout << "[DepthUpdater] status is GRPC_CHANNEL_CONNECTING" << endl;
                break;
            }
            case GRPC_CHANNEL_READY: {           
                std::cout << "[DepthUpdater] status is GRPC_CHANNEL_READY" << endl;
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                std::cout << "[DepthUpdater] status is GRPC_CHANNEL_TRANSIENT_FAILURE" << endl;
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                std::cout << "[DepthUpdater] status is GRPC_CHANNEL_SHUTDOWN" << endl;
                break;
            }
        }
        /*
        while (reader->Read(&multiQuote)) {
            // split and convert
            // std::cout << "get " << multiQuote.quotes_size() << " items" << std::endl;
            for( int i = 0 ; i < multiQuote.quotes_size() ; ++ i ) {
                const SEData& quote = multiQuote.quotes(i);
                // std::cout << "update symbol " << quote.symbol() << " " << quote.asks_size() << "/" << quote.bids_size() << std::endl;
                callback->on_raw_depth(quote);
            }
        }
        */
        while (reader->Read(&dataInBinary)) {
            const string& data = dataInBinary.data();            
            // cout << "Origianl Data: " << data << endl;    
            if( !this->_decode_one_package(callback, data) )
            {
                cout << "Decode Data Failed" << endl;
                break;
            }
                
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "SubscribeQuote rpc succeeded." << std::endl;
        } else {
            std::cout << "SubscribeQuote rpc failed." << std::endl;
        }
    }

    bool _decode_one_package(IStreamEngineUpdater* callback, const string& data)
    {
        std::string::size_type startpos = 0;
        while( startpos < data.size() )
        {
            std::string::size_type pos = data.find(";", startpos);
            if( pos == std::string::npos )
                return false;
            std::string::size_type pos2 = data.find(";", pos+1);
            if( pos2 == std::string::npos )
                return false;
            
            // length
            uint64 length = ToUint64(data.substr(startpos, pos));

            // datatype
            uint64 datatype = ToUint64(data.substr(pos+1, pos2-pos-1));

            string quoteData = data.substr(pos2+1, length);

            if( datatype == QUOTE_TYPE_DEPTH ) {
                SEData quote;
                if( !quote.ParseFromString(quoteData) )
                    return false;
                callback->on_raw_depth(quote);
            } else if( datatype == QUOTE_TYPE_TRADE ) {
                SETrade trade;
                if( !trade.ParseFromString(quoteData) )
                    return false;
                callback->on_trade(trade);
            }
            startpos = pos2 + 1 + length;
            //cout << startpos << ", " << data.size() << endl;
        } 
        return true;
    }

    void _run(const string& addr, IStreamEngineUpdater* callback) {
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};