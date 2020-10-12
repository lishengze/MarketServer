#include "grpc_server.h"

void ServerImpl::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    markets_[exchange][symbol] = quote;
};

void ServerImpl::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote)
{

};

void ServerImpl::on_mix_snap(const string& symbol, const SMixQuote& quote)
{
    std::shared_ptr<QuoteData> ptrQuote;
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
        iter->first->add_data(ptrQuote);
    }
};

bool ServerImpl::get_snap(const string& exchange, const string& symbol, SDepthQuote& quote) const
{
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    return _get_quote(exchange, symbol, quote);
};

void ServerImpl::register_client(CallDataMultiSubscribeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    clients_[calldata] = true;
}

void ServerImpl::unregister_client(CallDataMultiSubscribeQuote* calldata)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    auto iter = clients_.find(calldata);
    if( iter != clients_.end() ) {
        clients_.erase(iter);
    }
}
