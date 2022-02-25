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
#include "risk_controller.grpc.pb.h"

#include "Log/log.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::RiskController;
using SEMultiData = quote::service::v1::MultiMarketStreamDataWithDecimal;
using SEData = quote::service::v1::MarketStreamDataWithDecimal;
using SEDepth = quote::service::v1::DepthWithDecimal;
using quote::service::v1::Decimal;

inline void Decimal_to_SDecimal(SDecimal& dst, const Decimal& src)
{
    dst.from_raw(src.base(), src.prec());
}

class IQuoteUpdater {
public:
    virtual void on_snap(const SEData& quote) = 0;
};

class QuoteUpdater 
{
public:
    QuoteUpdater(){}
    ~QuoteUpdater(){
        if (thread_loop_.joinable())
        {
            thread_loop_.join();
        }
    }

    void start(const string& addr, IQuoteUpdater* callback) {
        try
        {
            thread_loop_ = std::thread(&QuoteUpdater::_run, this, addr, callback);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

private:

    void _request(const string& addr, IQuoteUpdater* callback) {
        try
        {
            LOG_INFO("_request");
            auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
            std::unique_ptr<RiskController::Stub> stub = RiskController::NewStub(channel);

            google::protobuf::Empty req;
            SEMultiData multiQuote;
            ClientContext context;

            std::unique_ptr<ClientReader<SEMultiData> > reader(stub->ServeMarketStream4Client(&context, req));
            switch(channel->GetState(true)) {
                case GRPC_CHANNEL_IDLE: {
                    LOG_INFO("[QuoteUpdater] status is GRPC_CHANNEL_IDLE");
                    break;
                }
                case GRPC_CHANNEL_CONNECTING: {                
                    LOG_INFO("[QuoteUpdater] status is GRPC_CHANNEL_CONNECTING");
                    break;
                }
                case GRPC_CHANNEL_READY: {           
                    LOG_INFO("[QuoteUpdater] status is GRPC_CHANNEL_READY");
                    break;
                }
                case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                    LOG_ERROR("[QuoteUpdater] status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                    return;
                }
                case GRPC_CHANNEL_SHUTDOWN: {        
                    LOG_WARN("[QuoteUpdater] status is GRPC_CHANNEL_SHUTDOWN");
                    break;
                }
            }
            while (reader->Read(&multiQuote)) {
                // split and convert
                // std::cout << "get " << multiQuote.quotes_size() << " items" << std::endl;
                for( int i = 0 ; i < multiQuote.quotes_size() ; ++ i ) {
                    const SEData& quote = multiQuote.quotes(i);
                    // std::cout << "QuoteUpdater  symbol " << quote.symbol() << " " << quote.asks_size() << "/" << quote.bids_size() << std::endl;
                    callback->on_snap(quote);
                }
            }
            Status status = reader->Finish();
            if (status.ok()) {
                LOG_INFO("ServeMarketStream4Client rpc succeeded.");
            } else {
                LOG_ERROR("ServeMarketStream4Client rpc failed." );
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

    void _run(const string& addr, IQuoteUpdater* callback) {
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