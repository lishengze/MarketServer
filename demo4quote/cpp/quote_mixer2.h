#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class QuoteMixer2
{
public:
    QuoteMixer2();
    ~QuoteMixer2();
    
    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void clear_exchange(const TExchange& exchange);

    void change_precise(const TSymbol& symbol, int precise);
private:
    // 行情数据
    mutable std::mutex mutex_quotes_;
    unordered_map<TExchange, unordered_map<TSymbol, SDepthQuote>> singles_;
    void _snap_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output);
    void _update_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output);
    void _precess_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output);
    unordered_map<TSymbol, SMixQuote*> quotes_;
    unordered_map<TSymbol, SMixQuote*> quote_updates_; // 控制频率的增量，目前没有使用

    bool _on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap);
    bool _on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff);

    bool _get_quote(const TSymbol& symbol, SMixQuote*& ptr) const;

    SMixDepthPrice* _clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const map<SDecimal, double>& newDepths, bool isAsk);

    SMixDepthPrice* _clear_exchange(const TExchange& exchange, SMixDepthPrice* depths);

    SMixDepthPrice* _mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, double>>& depths, bool isAsk);

    // 发布聚合行情
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, type_tick> last_clocks_;
    bool _check_update_clocks(const TSymbol& symbol);
    void _publish_quote(const TSymbol& symbol, std::shared_ptr<MarketStreamData> pub_snap, std::shared_ptr<MarketStreamData> pub_diff, bool is_snap);
};