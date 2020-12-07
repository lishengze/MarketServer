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
#include "base/cpp/quote.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using quote::service::v1::StreamEngine;
using quote::service::v1::GetKlinesRequest;
using quote::service::v1::GetKlinesResponse;


class IKlineUpdater {
public:
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const KlineData& kline) = 0;
};


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
    ~KlineUpdater(){}

    void start(const string& addr, IKlineUpdater* callback) {
        thread_loop_ = new std::thread(&KlineUpdater::_run, this, addr, callback);
    }

private:

    void _request(const string& addr, IKlineUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

        GetKlinesRequest req;
        GetKlinesResponse resp;
        ClientContext context;

        std::unique_ptr<ClientReader<GetKlinesResponse> > reader(stub->GetLast(&context, req));
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
            // std::cout << "get " << multiQuote.quotes_size() << " items" << std::endl;
            const char* ptr = resp.data().c_str();
            size_t length = resp.data().length();
            if( length != sizeof(WrapperKlineData) * resp.num() ) {
                std::cout << "size not match(single=" << sizeof(WrapperKlineData) << ", total=" << resp.data().length() << "). reconnect service..." << endl;
                return;
            }
            const WrapperKlineData* data = (const WrapperKlineData*)ptr;
            for( int i = 0 ; i < resp.num() ; i ++ ) {
                const WrapperKlineData& tmp = data[i];
                KlineData kline;
                kline.index = tmp.index;
                kline.px_open = tmp.px_open;
                kline.px_high = tmp.px_high;
                kline.px_low = tmp.px_low;
                kline.px_close = tmp.px_close;
                kline.volume = tmp.volume;
                callback->on_kline(tmp.exchange, tmp.symbol, tmp.resolution, kline);
            }
        }
        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "GetLast rpc succeeded." << std::endl;
        } else {
            std::cout << "GetLast rpc failed." << std::endl;
        }
    }

    void _run(const string& addr, IKlineUpdater* callback) {
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};