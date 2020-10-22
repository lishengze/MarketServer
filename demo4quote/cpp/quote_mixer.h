#pragma once

#include "stream_engine_define.h"

void compress_quote(const string& symbol, const SDepthQuote& src, SDepthQuote& dst);

class QuoteMixer
{
public:
    QuoteMixer(){}

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void publish_quote(const string& symbol, const SMixQuote& quote, bool isSnap);

private:

    // symbols
    unordered_map<TSymbol, SMixQuote*> symbols_;
    unordered_map<TSymbol, long long> last_clocks_;

    bool _get_quote(const string& symbol, SMixQuote*& ptr) const;

    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _clear_exchange(const string& exchange, SMixDepthPrice* depths);

    void _cross_askbid(SMixQuote* mixedQuote, const SDepthQuote& quote);

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, const SDecimal& watermark, bool isAsk);
};