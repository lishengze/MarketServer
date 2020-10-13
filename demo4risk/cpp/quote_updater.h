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

#include "stream_engine_server.grpc.pb.h"
#include "pandora/util/singleton.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using trade::service::v1::StreamEngineService;
using trade::service::v1::GetQuoteReq;
using trade::service::v1::SubscribeQuoteReq;
using trade::service::v1::MultiQuoteData;
using trade::service::v1::QuoteData;
using trade::service::v1::DepthLevel;
using trade::service::v1::Decimal;
using trade::service::v1::DepthVolume;


class IQuoteUpdater {
public:
    virtual void on_snap(const QuoteData& quote) = 0;
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

    void _run(const string& addr, IQuoteUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<StreamEngineService::Stub> stub = StreamEngineService::NewStub(channel);

        SubscribeQuoteReq req;
        MultiQuoteData multiQuote;
        ClientContext context;

        std::unique_ptr<ClientReader<MultiQuoteData> > reader(stub->MultiSubscribeQuote(&context, req));
        while (reader->Read(&multiQuote)) {
            // split and convert
            // std::cout << "get " << multiQuote.quotes_size() << " items" << std::endl;
            for( int i = 0 ; i < multiQuote.quotes_size() ; ++ i ) {
                const QuoteData& quote = multiQuote.quotes(i);
                std::cout << "update symbol " << quote.symbol() << " " << quote.ask_depth_size() << "/" << quote.bid_depth_size() << std::endl;
                callback->on_snap(quote);
            }
        }
        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "MultiSubscribeQuote rpc succeeded." << std::endl;
        } else {
            std::cout << "MultiSubscribeQuote rpc failed." << std::endl;
        }
    }

    std::thread*               thread_loop_ = nullptr;
};