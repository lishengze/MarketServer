#pragma once

#include "redis_quote_snap.h"
#include "redis_quote_dumper.h"
#include "base/cpp/basic.h"

bool decode_channelname(const string& channel_name, string& channel_type, TSymbol& symbol, TExchange& exchange);

struct RedisKlineHelper
{
    uint32 resolution_;
    unordered_map<TSymbol, unordered_map<TExchange, type_tick>> first_package_;

    RedisKlineHelper(uint32 resolution): resolution_(resolution){};
};

class RedisQuote : public utrade::pandora::CRedisSpi, public QuoteSourceInterface
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
        unordered_map<TExchange, SymbolMeta> exchanges;        

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

    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisQuote();
    ~RedisQuote();

    // 继承自QuoteSourceInterface
    // ！不支持运行时变更精度
    bool set_config(const TSymbol& symbol, const SSymbolConfig& config);
    bool start();
    bool stop();

    // 继承自CRedisSpi
    virtual void OnConnected();
    virtual void OnDisconnected(int status);
    virtual void OnMessage(const std::string& channel, const std::string& msg) {
        bool retry = false;
        on_message(channel, msg, retry);
    }

    // 初始化连接redis
    void init(QuoteSourceCallbackInterface* callback, const RedisParams& params, UTLogPtr logger, bool dump);
    // 初始化回放
    void init_replay(QuoteSourceCallbackInterface* callback, int ratio, int replicas);

    // 订阅
    void subscribe(const TExchange& exchange, const TSymbol& symbol) {
        if( !connected_ )
            return;
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", DEPTH_UPDATE_HEAD, symbol, exchange));
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", TRADE_HEAD, symbol, exchange));
        redis_api_->SubscribeTopic(tfm::format("%s|%s.%s", KLINE_1MIN_HEAD, symbol, exchange));
    }
    void unsubscribe(const TExchange& exchange, const TSymbol& symbol) {
        if( !connected_ )
            return;
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", DEPTH_UPDATE_HEAD, symbol, exchange));
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", TRADE_HEAD, symbol, exchange));
        redis_api_->UnSubscribeTopic(tfm::format("%s|%s.%s", KLINE_1MIN_HEAD, symbol, exchange));
    }

    void on_message(const std::string& channel, const std::string& msg, bool& retry);

    SDepthQuote merge_update(list<SDepthQuote>& updates_queue, SDepthQuote& snap);

    SDepthQuote merge_quote(list<SDepthQuote>& quote_list);
    
private:
    std::map<TExchange, std::map<TSymbol, int>> exchange_symbol_depth_alive_map_;
    void check_exchange_symbol_depth_alive();
    void erase_dead_exchange_symbol_depth(const TExchange& exchange, const TSymbol& symbol);
    void set_exchange_symbol_depth_alive_map(const TExchange& exchange, const TSymbol& symbol);

    std::mutex mutex_check_;
private:
    // redis连接参数
    RedisParams params_;
    // 上一次连接时间
    type_tick last_redis_time_;
    // redis接口对象
    RedisApiPtr redis_api_;
    bool connected_ = false;

    // 管理exchange+symbol的基础信息
    mutable std::mutex mutex_metas_;
    unordered_map<TSymbol, ExchangeMeta> metas_;

    // redis snap requester
    RedisSnapRequester redis_snap_requester_;

    // sync snap and updater
    int SYNC_OK{0};
    int SYNC_STARTING{1};
    int SYNC_SKIP{2};
    int SYNC_REQ_SNAP{3};
    int _sync_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& snap);
    int _sync_by_snap(const TExchange& exchange, const TSymbol& symbol, SDepthQuote& quote, list<SDepthQuote>& updates_queue);
    
    // callback
    QuoteSourceCallbackInterface *engine_interface_ = nullptr;

    // 独立线程检查redis数据通道
    //std::mutex mutex_checker_;
    type_tick last_time_; // 上一次从redis收到行情的时间
    std::thread* checker_loop_ = nullptr;
    std::atomic<bool> thread_run_;
    void _looping();
    void _loopng_check_heartbeat();
    type_tick last_statistic_time_; // 上一次计算统计信息时间
    void _looping_print_statistics();
    type_tick last_nodata_time_; // 上一次检查交易所数据的时间
    void _looping_check_nodata();
    void _looping_force_to_update();

    string last_statistic_time_str_;
    int  _looping_check_secs{5};
    map<string, int>    _statistic_map;


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

    // 录取/回放行情源头数据
    bool dump_ = false;
    QuoteDumper quote_dumper_;
    bool replay_ = false;
    QuoteReplayer quote_replayer_;
};