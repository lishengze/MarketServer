#pragma once

#include "stream_engine_define.h"
#include "grpc_entity.h"

class IMixerQuotePusher
{
public:
    virtual void publish_single(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    virtual void publish_mix(const TSymbol& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap) = 0;
    virtual void publish_trade(const TExchange& exchange, const TSymbol& symbol, std::shared_ptr<TradeWithDecimal> trade) = 0;
};

class IMixerCacher
{
public:
    virtual bool get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps) = 0;
};

class QuoteMixer2 : public IMixerCacher
{
public:
    struct SSymbolConfig
    {
        type_uint32 depth;
        type_uint32 precise;
        type_uint32 vprecise;
        float frequency;
        unordered_map<TExchange, SymbolFee> fees;

        bool operator==(const SSymbolConfig &rhs) const {
            return depth == rhs.depth && precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency&& fees == rhs.fees;
        }
        bool operator!=(const SSymbolConfig &rhs) const {
            return !(*this == rhs);
        }
    };
public:
    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void set_config(const TSymbol& symbol, const SSymbolConfig& config);

    void register_callback(IMixerQuotePusher* callback) { callbacks_.insert(callback); }

    bool get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps);
private:
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
    unordered_map<TSymbol, type_tick> last_clocks_;
    bool _check_update_clocks(const TSymbol& symbol, float frequency);

    mutable std::mutex mutex_config_;
    unordered_map<TSymbol, SSymbolConfig> configs_; 
};

/*
缓存接口
*/
class IQuoteCacher
{
public:
    // 请求缓存中的K线
    virtual bool get_latetrades(vector<TradeWithDecimal>& trades) = 0;
    virtual bool get_lastsnap(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps) = 0;
};

class QuoteCacher : public IQuoteCacher
{
public:
    void set_mixer(QuoteMixer2* mixer) { mixer_ = mixer; }

    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    void register_callback(IMixerQuotePusher* callback) { callbacks_.insert(callback); }

    void clear_exchange(const TExchange& exchange);

    bool get_latetrades(vector<TradeWithDecimal>& trades);

    bool get_lastsnap(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps);
private:
    set<IMixerQuotePusher*> callbacks_;

    QuoteMixer2* mixer_ = nullptr;

    mutable std::mutex mutex_quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> singles_;
    unordered_map<TSymbol, unordered_map<TExchange, Trade>> trades_;
};
