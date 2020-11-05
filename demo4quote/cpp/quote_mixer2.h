#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class QuoteMixer2
{
public:
    QuoteMixer2();
    ~QuoteMixer2();
    
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

private:
    bool _preprocess(const string& exchange, const string& symbol, const SDepthQuote& src, SDepthQuote& dst);

    // quote
    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, SMixQuote*> quotes_;

    bool _on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap);
    bool _on_update(const string& exchange, const string& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff);

    bool _get_quote(const string& symbol, SMixQuote*& ptr) const;

    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _clear_exchange(const string& exchange, SMixDepthPrice* depths);

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk);

    // 发布聚合行情
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, type_tick> last_clocks_;
    bool _check_update_clocks(const string& symbol);
    void _publish_quote(const string& symbol, std::shared_ptr<MarketStreamData> pub_snap, std::shared_ptr<MarketStreamData> pub_diff, bool is_snap);
};