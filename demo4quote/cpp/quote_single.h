#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class QuoteSingle
{
public:
    QuoteSingle(){}

    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void clear_exchange(const TExchange& exchange);
private:
    bool _on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap);
    bool _on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff);

    mutable std::mutex mutex_quotes_;
    unordered_map<TExchange, unordered_map<TSymbol, SMixQuote*>> quotes_;

    bool _get_quote(const TExchange& exchange, const TSymbol& symbol, SMixQuote*& ptr) const;
    
    SMixDepthPrice* _clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk);
};
