#pragma once
#include "kline_mixer.h"
#include "base/cpp/grpc_call.h"
#include "kline_server.grpc.pb.h"

using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using quote::service::v1::GetKlinesResponse;
using quote::service::v1::GetKlinesRequest;
using GrpcKlineService = quote::service::v1::KlineServer;

class GetKlinesEntity : public BaseGrpcEntity
{
public:
    GetKlinesEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    GetKlinesEntity* spawn() {
        return new GetKlinesEntity(service_, provider_);
    }

private:
    GrpcKlineService::AsyncService* service_;
    ServerContext ctx_;

    GetKlinesRequest request_;
    GetKlinesResponse reply_;
    ServerAsyncResponseWriter<GetKlinesResponse> responder_;

    IDataProvider* provider_;
};

class GetLastEntity : public BaseGrpcEntity
{
public:
    GetLastEntity(void* service, IDataProvider* provider);

    void register_call();

    bool process();

    void add_data(const KlineData& kline) {        
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        datas_.push_back(kline);
        if( total_.size() == 0 || total_.back().index < kline.index ) {
            total_.push_back(kline);
        } else if( total_.back().index == kline.index ) {
            total_.back() = kline;
        }
    }

    GetLastEntity* spawn() {
        return new GetLastEntity(service_, provider_);
    }
private:
    GrpcKlineService::AsyncService* service_;

    ServerContext ctx_;

    GetKlinesRequest request_;
    ServerAsyncWriter<GetKlinesResponse> responder_;

    // 
    mutable std::mutex            mutex_datas_;
    list<KlineData> datas_;
    list<KlineData> total_;
    
    IDataProvider* provider_;
};
