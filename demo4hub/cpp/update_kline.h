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

#include "pandora/util/time_util.h"
#include "Log/log.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::StreamEngine;
using quote::service::v1::GetKlinesRequest;
using quote::service::v1::GetKlinesResponse;
using quote::service::v1::MultiGetKlinesResponse;
using SEKlineData = quote::service::v1::GetKlinesResponse;
using SEKline = quote::service::v1::Kline;


class IKlineUpdater {
public:
    virtual void on_kline(const SEKlineData& quote) = 0;
};

inline void RequestKline(const string& addr, const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start, type_tick end, vector<SEKline>& klines)
{
    klines.clear();

    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);
    
    GetKlinesRequest req;
    req.set_exchange(exchange);
    req.set_symbol(symbol);
    req.set_resolution(resolution);
    req.set_start_time(start);
    req.set_end_time(end);
    GetKlinesResponse resp;
    ClientContext context;

    stub->GetKlines(&context, req, &resp);

    for( int i = 0 ; i < resp.klines_size() ; i ++ ) {
        const SEKline& kline = resp.klines(i);
        klines.push_back(kline);
    }
}

class KlineUpdater 
{
public:
    struct WrapperKlineData: KlineData
    {
        char exchange[16];
        char symbol[16];
        int resolution;
    };

public:
    KlineUpdater(){}
    ~KlineUpdater(){

        if (thread_loop_.joinable())
        {
            thread_loop_.join();
        }        
    }

    void start(const string& addr, IKlineUpdater* callback) {
        try
        {
            thread_loop_ = std::thread(&KlineUpdater::_run, this, addr, callback);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

private:

    void _request(const string& addr, IKlineUpdater* callback) 
    {
        try
        {
            LOG_INFO(" [KlineUpdater] Start Request");
            
            auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
            std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

            GetKlinesRequest req;
            MultiGetKlinesResponse resp;
            ClientContext context;

            std::unique_ptr<ClientReader<MultiGetKlinesResponse> > reader(stub->GetLast(&context, req));
            switch(channel->GetState(true)) {
                case GRPC_CHANNEL_IDLE: {
                    LOG_INFO("[KlineUpdater] status is GRPC_CHANNEL_IDLE");
                    break;
                }
                case GRPC_CHANNEL_CONNECTING: {        
                    LOG_INFO("[KlineUpdater] status is GRPC_CHANNEL_CONNECTING");
                    break;
                }
                case GRPC_CHANNEL_READY: {           
                    LOG_INFO("[KlineUpdater] status is GRPC_CHANNEL_READY");
                    break;
                }
                case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                    LOG_ERROR("[KlineUpdater] status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                    return;
                }
                case GRPC_CHANNEL_SHUTDOWN: {        
                    LOG_ERROR("[KlineUpdater] status is GRPC_CHANNEL_SHUTDOWN");
                    break;
                }
            }
            while (reader->Read(&resp)) 
            {   
                LOG_TRACE(" [update_kline] get " + std::to_string(resp.data_size()) + " items ****");
                for( int i = 0 ; i < resp.data_size() ; i ++ )
                {
                    const SEKlineData& quote = resp.data(i);
                    callback->on_kline(quote);
                }
            }

            Status status = reader->Finish();
            if (status.ok()) {
                LOG_INFO("GetLast rpc succeeded");
            } else {
                LOG_ERROR("GetLast rpc failed");
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
        

    }

    void _run(const string& addr, IKlineUpdater* callback) {
        try
        {
            while( 1 ) {            
                _request(addr, callback);
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

    std::thread               thread_loop_;
};