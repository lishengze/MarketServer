#pragma once

#include "stream_engine_define.h"

class QuoteSingle
{
public:
    QuoteSingle(){}

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void publish_quote(const string& exchange, const string& symbol, const SMixQuote& quote, bool isSnap);

private:
    unordered_map<TExchange, unordered_map<TSymbol, SMixQuote*>> symbols_;
    unordered_map<TExchange, unordered_map<TSymbol, long long>> last_clocks_;

    bool _get_quote(const string& exchange, const string& symbol, SMixQuote*& ptr) const;
    
    SMixDepthPrice* _clear_allpricelevel(const string& exchange, SMixDepthPrice* depths);
    
    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk);
};
