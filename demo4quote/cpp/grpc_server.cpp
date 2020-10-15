#include "stream_engine_config.h"
#include "grpc_server.h"


void mixquote_to_pbquote(const string& symbol, const SMixQuote& quote, QuoteData* msd) {
    msd->set_symbol(symbol);

    // 卖盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = quote.Asks;
        while( ptr != NULL && depth_count < CONFIG->grpc_push_depth_ ) {
            DepthLevel* depth = msd->add_ask_depth();
            depth->mutable_price()->set_value(ptr->Price.Value);
            depth->mutable_price()->set_base(ptr->Price.Base);
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthVolume* depthVolume = depth->add_data();
                depthVolume->set_volume(volume);
                depthVolume->set_exchange(exchange);
            }
            depth_count += 1;
            ptr = ptr->Next;
        }
    }
    // 买盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = quote.Bids;
        while( ptr != NULL && depth_count < CONFIG->grpc_push_depth_ ) {
            DepthLevel* depth = msd->add_bid_depth();
            depth->mutable_price()->set_value(ptr->Price.Value);
            depth->mutable_price()->set_base(ptr->Price.Base);
            for(auto &v : ptr->Volume) {
                const TExchange& exchange = v.first;
                const double volume = v.second;
                DepthVolume* depthVolume = depth->add_data();
                depthVolume->set_volume(volume);
                depthVolume->set_exchange(exchange);
            }
            depth_count += 1;
            ptr = ptr->Next;
        }
    }
};

void GrpcServer::on_snap(const string& exchange, const string& symbol, const SMixQuote& quote)
{
    // publish
    std::shared_ptr<QuoteData> ptrQuote(new QuoteData());
    mixquote_to_pbquote(symbol, quote, ptrQuote.get());
    std::unique_lock<std::mutex> inner_lock{ mutex_clients2_ };
    for( auto iter = clients2_.begin() ; iter != clients2_.end() ; ++iter ) {
        iter->first->add_data(exchange, ptrQuote);
    }
};

void GrpcServer::on_mix_snap(const string& symbol, const SMixQuote& quote)
{
    std::shared_ptr<QuoteData> ptrQuote(new QuoteData());
    mixquote_to_pbquote(symbol, quote, ptrQuote.get());
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
        iter->first->add_data(ptrQuote);
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
