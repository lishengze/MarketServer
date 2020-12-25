#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class IMixerQuotePusher
{
public:
    virtual void publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update) = 0;
    virtual void publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update) = 0;
};

class QuoteMixer2
{
public:
    QuoteMixer2();
    ~QuoteMixer2();
    
    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    void clear_exchange(const TExchange& exchange);

    void register_callback(IMixerQuotePusher* callback) { callbacks_.insert(callback); }

    void set_publish_params(const TSymbol& symbol, float frequency);
    void set_compute_params(const TSymbol& symbol, int precise, type_uint32 depth, const unordered_map<TExchange, SymbolFee>& fees);
private:
    set<IMixerQuotePusher*> callbacks_;

    // 行情数据
    struct _compute_param{
        type_uint32 depth;
        int precise;
        unordered_map<TExchange, SymbolFee> fees;
    };
    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, _compute_param> _compute_params_; 
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> singles_;
    void _snap_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output);
    void _update_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output);
    void _precess_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, int precise, const SymbolFee& fee, SDepthQuote& output);
    void _inner_process(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SMixQuote* ptr);
    unordered_map<TSymbol, SMixQuote*> quotes_;
    unordered_map<TSymbol, SMixQuote*> quote_updates_; // 控制频率的增量，目前没有使用

    bool _on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamDataWithDecimal>& pub_snap);
    bool _on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamDataWithDecimal>& pub_snap, std::shared_ptr<MarketStreamDataWithDecimal>& pub_diff);

    bool _get_quote(const TSymbol& symbol, SMixQuote*& ptr) const;

    SMixDepthPrice* _clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const map<SDecimal, SDecimal>& newDepths, bool isAsk);

    SMixDepthPrice* _clear_exchange(const TExchange& exchange, SMixDepthPrice* depths);

    SMixDepthPrice* _mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, SDecimal>>& depths, bool isAsk);

    // 发布聚合行情
    struct _publish_param{
        float frequency;
    };
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, _publish_param> publish_params_; 
    unordered_map<TSymbol, type_tick> last_clocks_;
    bool _check_update_clocks(const TSymbol& symbol, float frequency);
    void _publish_quote(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> pub_snap, std::shared_ptr<MarketStreamDataWithDecimal> pub_diff, bool is_snap);
};