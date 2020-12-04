#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"
using namespace std;
using njson = nlohmann::json;
#include "redis_quote_snap.h"
#include "nacos_client.h"

struct SymbolMeta
{    
    int pkg_count; // 收到包的数量
    type_seqno seq_no; // 最近一次有效的序列号
    list<SDepthQuote> caches; // 缓存中的序列号 
    SDepthQuote snap;

    static const int MAX_SIZE = 1000;

    bool publish_started() const { return seq_no > 0 ; }
    
    SymbolMeta() {
        pkg_count = 0;
        seq_no = 0;
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
        char content[1024];
        if( pkg_count > 0 ) {
            sprintf(content, "%d\t\t%d\t\t%d", pkg_count, pkg_size, int(pkg_size/pkg_count));
        } else {
            sprintf(content, "%d\t\t%d\t\t-", pkg_count, pkg_size);
        }
        return content;
    }

    void accumlate(const ExchangeMeta& stat) {
        pkg_count += stat.pkg_count;
        pkg_size += stat.pkg_size;
    }
};

class RedisQuote : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisQuote(){};
    ~RedisQuote();

    // 动态修改配置
    void set_frequency(const TSymbol& symbol, float frequency);
    void set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    // init
    void start(const RedisParams& params, UTLogPtr logger);
    void set_engine(QuoteSourceInterface* ptr) { engine_interface_ = ptr; }
    void subscribe(const TExchange& exchange, const TSymbol& symbol) {
        redis_api_->SubscribeTopic("UPDATEx|" + symbol + "." + exchange);
        redis_api_->SubscribeTopic("KLINEx|" + symbol + "." + exchange);
    }
    void unsubscribe(const TExchange& exchange, const TSymbol& symbol) {
        redis_api_->UnSubscribeTopic("UPDATEx|" + symbol + "." + exchange);
        redis_api_->UnSubscribeTopic("KLINEx|" + symbol + "." + exchange);
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
    RedisParams params_;
    // 上一次连接时间
    type_tick last_redis_time_;
    // redis接口对象
    RedisApiPtr     redis_api_;
    set<string>     subscribed_topics_;

    // 管理exchange+symbol的基础信息
    mutable std::mutex mutex_metas_;
    unordered_map<TExchange, ExchangeMeta> metas_;

    // redis snap requester
    RedisSnapRequester    redis_snap_requester_;

    // sync snap and updater
    bool _update_meta_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);
    bool _update_meta_by_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, list<SDepthQuote>& wait_to_send);
    bool _check_snap_received(const TExchange& exchange, const TSymbol& symbol) const;

    
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;

    // 独立线程检查redis数据通道
    //std::mutex mutex_checker_;
    type_tick last_time_; // 上一次从redis收到行情的时间
    std::thread* checker_loop_ = nullptr;
    void _check();

    // 统计信息
    type_tick last_statistic_time_; // 上一次计算统计信息时间
    type_tick last_nodata_time_; // 上一次检查交易所数据的时间

    // 行情源头下发速度控制    
    mutable std::mutex mutex_clocks_;   
    unordered_map<TSymbol, float> frequecy_; 
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> updates_;  // 增量更新数据
    unordered_map<TSymbol, unordered_map<TExchange, type_tick>> last_clocks_;
    bool _check_update_clocks(const TExchange& exchange, const TSymbol& symbol, float frequency);
    bool _ctrl_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    // 订阅的交易币种换粗
    unordered_map<TSymbol, unordered_set<TExchange>> symbols_;
};