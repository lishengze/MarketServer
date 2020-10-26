#include "stream_engine_config.h"
#include "grpc_server.h"

void GrpcServer::on_snap(const string& exchange, const string& symbol, std::shared_ptr<QuoteData> quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_clients2_ };
    for( auto iter = clients2_.begin() ; iter != clients2_.end() ; ++iter ) {
        iter->first->add_data(exchange, quote);
    }
};

void GrpcServer::on_mix_snap(const string& symbol, std::shared_ptr<QuoteData> quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
        iter->first->add_data(quote);
    }
};

void GrpcServer::on_mix_snap4hedge(const string& symbol, std::shared_ptr<QuoteData> quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_clients3_ };
    for( auto iter = clients3_.begin() ; iter != clients3_.end() ; ++iter ) {
        iter->first->add_data(quote);
    }
};

void GrpcServer::register_client(CallDataMultiSubscribeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    clients_[calldata] = true;
}

void GrpcServer::unregister_client(CallDataMultiSubscribeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    auto iter = clients_.find(calldata);
    if( iter != clients_.end() ) {
        clients_.erase(iter);
    }
}


void GrpcServer::register_client2(CallDataSubscribeOneQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients2_ };
    clients2_[calldata] = true;
}

void GrpcServer::unregister_client2(CallDataSubscribeOneQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients2_ };
    auto iter = clients2_.find(calldata);
    if( iter != clients2_.end() ) {
        clients2_.erase(iter);
    }
}



void GrpcServer::register_client3(CallDataMultiSubscribeHedgeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients3_ };
    clients3_[calldata] = true;
}

void GrpcServer::unregister_client3(CallDataMultiSubscribeHedgeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients3_ };
    auto iter = clients3_.find(calldata);
    if( iter != clients3_.end() ) {
        clients3_.erase(iter);
    }
}
