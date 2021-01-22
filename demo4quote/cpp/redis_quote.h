#pragma once

#include "redis_quote_snap.h"

struct RedisKlineHelper
{
    uint32 resolution_;
    unordered_map<TSymbol, unordered_map<TExchange, bool>> first_package_;

    RedisKlineHelper(uint32 resolution): resolution_(resolution){};

    bool on_get_message(const njson& body, const TExchange& exchange, const TSymbol& symbol, const SExchangeConfig& config, vector<KlineData>& klines);
};

class RedisQuote : public utrade::pandora::CRedisSpi
{
public:
    
    struct SymbolMeta
    {    
        int pkg_count; // 收到包的数量
        type_seqno seq_no; // 最近的序号
        list<SDepthQuote> caches; // 缓存中的增量包
        SDepthQuote snap; // 缓存中的全量包
        bool publishing; // 是否开始推送

        static const int MAX_SIZE = 1000;
        
        SymbolMeta() {
            pkg_count = 0;
            seq_no = 0;
            publishing = false;
        }
    };

    struct ExchangeMeta
    {
        int pkg_count; // 收到包的数量
        int pkg_size;  // 收到包的大小
        int pkg_skip_count; // 丢包次数
        unordered_map<TSymbol, SymbolMeta> symbols;        

        ExchangeMeta() {
            reset();
        }

        void reset() {
            pkg_count = 0;
            pkg_size = 0;
            pkg_skip_count = 0;
        }
        
        string get() const {
            if( pkg_count > 0 ) {
                return tfm::format("%d\t\t%d\t\t%d", pkg_count, pkg_size, int(pkg_size/pkg_count));
            } else {
                return tfm::format("%d\t\t%d\t\t-", pkg_count, pkg_size);
            }
        }

        void accumlate(const ExchangeMeta& stat) {
            pkg_count += stat.pkg_count;
            pkg_size += stat.pkg_size;
        }
    };

    using SSymbolConfig = unordered_map<TExchange, SExchangeConfig>;
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisQuote();
    ~RedisQuote();

    // 动态修改配置
    void set_config(const TSymbol& symbol, const SSymbolConfig& config);

    // 初始化
    void init(const RedisParams& params, UTLogPtr logger, QuoteSourceInterface* callback);
    
    // 启动
    void start();

    // 订阅
    void subscribe(const TExchange& exchange, const TSymbol& symbol) {
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", DEPTH_UPDATE_HEAD, symbol, exchange));
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", TRADE_HEAD, symbol, exchange));
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", KLINE_1MIN_HEAD, symbol, exchange));
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", KLINE_60MIN_HEAD, symbol, exchange));
    }
    void unsubscribe(const TExchange& exchange, const TSymbol& symbol) {
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", DEPTH_UPDATE_HEAD, symbol, exchange));
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", TRADE_HEAD, symbol, exchange));
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", KLINE_1MIN_HEAD, symbol, exchange));
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", KLINE_60MIN_HEAD, symbol, exchange));
    }
    
    // callback from RedisSnapRequester
    bool _on_snap(const TExchange& exchange, const TSymbol& symbol, const string& data);

    // redis connect notify
    virtual void OnConnected();    
    // redis disconnect notify
    virtual void OnDisconnected(int status);
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    // redis连接参数
    RedisParams params_;
    // 上一次连接时间
    type_tick last_redis_time_;
    // redis接口对象
    RedisApiPtr redis_api_;

    // 管理exchange+symbol的基础信息
    mutable std::mutex mutex_metas_;
    unordered_map<TExchange, ExchangeMeta> metas_;

    // redis snap requester
    RedisSnapRequester redis_snap_requester_;

    // sync snap and updater
    int SYNC_OK{0};
    int SYNC_STARTING{1};
    int SYNC_SKIP{2};
    int SYNC_SNAPAGAIN{3};
    int _sync_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& snap);
    int _sync_by_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, list<SDepthQuote>& updates_queue);
    
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;

    // 独立线程检查redis数据通道
    //std::mutex mutex_checker_;
    type_tick last_time_; // 上一次从redis收到行情的时间
    std::thread* checker_loop_ = nullptr;
    void _looping();
    void _loopng_check_heartbeat();
    type_tick last_statistic_time_; // 上一次计算统计信息时间
    void _looping_print_statistics();
    type_tick last_nodata_time_; // 上一次检查交易所数据的时间
    void _looping_check_nodata();
    void _looping_force_to_update();


    // 行情源头下发速度控制    
    struct _SFrequencyMeta
    {
        unordered_map<TExchange, SDepthQuote> updates;
        unordered_map<TExchange, type_tick> last_clocks;
    };
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, _SFrequencyMeta> frequency_metas_;
    bool _ctrl_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, const SExchangeConfig& config, SDepthQuote& update);

    // 币种配置信息
    mutable std::mutex mutex_symbol_;
    unordered_map<TSymbol, SSymbolConfig> symbols_;
    bool _get_config(const TExchange& exchange, const TSymbol& symbol, SExchangeConfig& config) const;

    // K线更新相关
    RedisKlineHelper kline_min1_;
    RedisKlineHelper kline_min60_;
};