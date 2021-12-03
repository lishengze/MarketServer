#pragma once

#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"
#include "struct_define.h"

class IMixerQuotePusher
{
public:
    //virtual void publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    //virtual void publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    virtual void publish_trade(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<TradeWithDecimal> trade) = 0;
    virtual void publish_binary(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
};

class DepthAggregater
{
public:

public:

    DepthAggregater();
    ~DepthAggregater();

    void start();

    void set_engine(QuoteSourceCallbackInterface* ptr) { engine_interface_ = ptr; }

    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    void set_config(const TSymbol& symbol, const SMixerConfig& config);

    //void register_callback(IMixerQuotePusher* callback) { callbacks_.insert(callback); }

    //bool get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps);
private:
    // callback
    QuoteSourceCallbackInterface *engine_interface_ = nullptr;

    // 计算线程
    std::thread*        thread_loop_ = nullptr;
    std::atomic<bool>   thread_run_;
    void _thread_loop();

    // 配置信息
    mutable std::mutex mutex_config_;
    unordered_map<TSymbol, SMixerConfig> configs_; 

    // 控制频率    
    unordered_map<TSymbol, type_tick> last_clocks_;

    // 缓存数据
    void _calc_symbol(const TSymbol& symbol, const SMixerConfig& config, type_seqno seqno);
    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, Trade>> trades_;
    /*
    set<IMixerQuotePusher*> callbacks_;

    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, SMixQuote*> quotes_;

    bool _get_quote(const TSymbol& symbol, SMixQuote*& ptr) const;
    void _inner_process(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SMixQuote* ptr);
    SMixDepthPrice* _clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const map<SDecimal, SDepth>& newDepths, bool isAsk);
    SMixDepthPrice* _clear_exchange(const TExchange& exchange, SMixDepthPrice* depths);
    SMixDepthPrice* _mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, SDepth>>& depths, bool isAsk);

    // 发布频率控制
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, type_tick> last_snap_clocks_;
    unordered_map<TSymbol, type_tick> last_trade_clocks_;
    bool _check_clocks(const TSymbol& symbol, float frequency, unordered_map<TSymbol, type_tick>& clocks);

    mutable std::mutex mutex_config_;
    unordered_map<TSymbol, SSymbolConfig> configs_; 
    */
};
