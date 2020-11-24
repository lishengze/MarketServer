#pragma once

#include "kline_mixer.h"
// grpc
#include "grpc_entity.h"

class ServerEndpoint : public IMixerKlinePusher
{
public:
    ServerEndpoint();
    ~ServerEndpoint();

    void init();

    void start();

    void set_provider(IDataProvider* provider) { provider_ = provider; }

    // IMixerKlinePusher
    void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines);
private:

    std::thread* thread_loop_ = nullptr;

    IDataProvider* provider_ = nullptr;

    // grpcc
    void _handle_rpcs();    
    std::unique_ptr<ServerCompletionQueue> cq_;
    GrpcKlineService::AsyncService service_;
    std::unique_ptr<Server> server_;
    GrpcCall<GetKlinesEntity>* caller_getklines_;
    GrpcCall<GetLastEntity>* caller_getlast_;
    unordered_map<int, CommonGrpcCall*> callers_;
};
