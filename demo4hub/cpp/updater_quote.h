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

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::RiskController;
using SEMultiData = quote::service::v1::MultiMarketStreamData;
using SEData = quote::service::v1::MarketStreamData;
using SEDepth = quote::service::v1::Depth;


class IQuoteUpdater {
public:
    virtual void on_snap(const SEData& quote) = 0;
};

class QuoteUpdater 
{
public:
    QuoteUpdater(){}
    ~QuoteUpdater(){}

    void start(const string& addr, IQuoteUpdater* callback) {
        thread_loop_ = new std::thread(&QuoteUpdater::_run, this, addr, callback);
    }

private:

    void _request(const string& addr, IQuoteUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<RiskController::Stub> stub = RiskController::NewStub(channel);

        google::protobuf::Empty req;
        SEMultiData multiQuote;
        ClientContext context;

        std::unique_ptr<ClientReader<SEMultiData> > reader(stub->ServeMarketStream4Client(&context, req));
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
        while (reader->Read(&multiQuote)) {
            // split and convert
            // std::cout << "get " << multiQuote.quotes_size() << " items" << std::endl;
            for( int i = 0 ; i < multiQuote.quotes_size() ; ++ i ) {
                const SEData& quote = multiQuote.quotes(i);
                std::cout << "update symbol " << quote.symbol() << " " << quote.asks_size() << "/" << quote.bids_size() << std::endl;
                callback->on_snap(quote);
            }
        }
        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "ServeMarketStream4Client rpc succeeded." << std::endl;
        } else {
            std::cout << "ServeMarketStream4Client rpc failed." << std::endl;
        }
    }

    void _run(const string& addr, IQuoteUpdater* callback) {
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};