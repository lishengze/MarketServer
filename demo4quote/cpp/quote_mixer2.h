#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class IMixerQuotePusher
{
public:
    //virtual void publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    //virtual void publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    virtual void publish_trade(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<TradeWithDecimal> trade) = 0;
    virtual void publish_binary(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
};

struct SMixerConfig
{
    type_uint32 depth;
    type_uint32 precise;
    type_uint32 vprecise;
    type_uint32 aprecise;
    float frequency;
    unordered_map<TExchange, SymbolFee> fees;

    std::string str() 
    {
        return "pre: " + std::to_string(precise) + ", vpre: " + std::to_string(vprecise) + ", apre: " +  std::to_string(aprecise);
    }

    bool operator==(const SMixerConfig &rhs) const {
        return depth == rhs.depth && precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency&& fees == rhs.fees;
    }
    bool operator!=(const SMixerConfig &rhs) const {
        return !(*this == rhs);
    }
};

class QuoteMixer2
{
public:

public:

    QuoteMixer2();
    ~QuoteMixer2();

    void start();

    void set_engine(QuoteSourceCallbackInterface* ptr) { engine_interface_ = ptr; }

    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    void set_config(const TSymbol& symbol, const SMixerConfig& config);

    void erase_dead_exchange_symbol_depth(const TExchange& exchange, const TSymbol& symbol);

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

/*
缓存接口
*/
class IQuoteCacher
{
public:
    // 请求缓存中的K线
    virtual bool get_latetrades(vector<std::shared_ptr<TradeWithDecimal>>& trades) = 0;
    virtual bool get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps, const TExchange* fix_exchange = NULL) = 0;
};

class QuoteCacher : public IQuoteCacher
{
public:
    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& snap);

    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    void register_callback(IMixerQuotePusher* callback) { callbacks_.insert(callback); }

    void clear_exchange(const TExchange& exchange);

    bool get_latetrades(vector<std::shared_ptr<TradeWithDecimal>>& trades);

    bool get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps, const TExchange* fix_exchange = NULL);

    void erase_dead_exchange_symbol_depth(const TExchange& exchange, const TSymbol& symbol);

private:
    uint32 publish_depths_ = 1000; // 仅向下游发布有限的档位
    
    set<IMixerQuotePusher*> callbacks_;

    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> singles_;
    unordered_map<TSymbol, unordered_map<TExchange, Trade>> trades_;
};
