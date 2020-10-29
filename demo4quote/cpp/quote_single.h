#pragma once

#include "stream_engine_define.h"

class QuoteSingle
{
public:
    QuoteSingle(){}

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

private:
    mutable std::mutex mutex_quotes_;
    unordered_map<TExchange, unordered_map<TSymbol, SMixQuote*>> quotes_;

    bool _get_quote(const string& exchange, const string& symbol, SMixQuote*& ptr) const;
    
    SMixDepthPrice* _clear_allpricelevel(const string& exchange, SMixDepthPrice* depths);
    
    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk);
        
    mutable std::mutex mutex_clocks_;
    unordered_map<TExchange, unordered_map<TSymbol, long long>> last_clocks_;
    bool _check_update_clocks(const TExchange& exchange, const TSymbol& symbol);
    void _publish_quote(const TExchange& exchange, const TSymbol& symbol, const SMixQuote* snap, const SDepthQuote* update, bool is_snap);
};
