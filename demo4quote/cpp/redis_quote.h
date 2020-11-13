#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"
using namespace std;
using njson = nlohmann::json;
#include "redis_quote_snap.h"

struct ExchangeStatistics
{
    int pkg_count;
    int pkg_size;

    ExchangeStatistics() {
        reset();
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

    void accumlate(const ExchangeStatistics& stat) {
        pkg_count += stat.pkg_count;
        pkg_size += stat.pkg_size;
    }

    void reset() {
        pkg_count = 0;
        pkg_size = 0;
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

    // init
    void start(const RedisParams& params, UTLogPtr logger);
    void set_engine(QuoteSourceInterface* ptr) { engine_interface_ = ptr; }
    void subscribe(const string& channel);
    void psubscribe(const string& pchannel);
    
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
    // 市场序号
    unordered_map<TExchange, unordered_map<TSymbol, type_seqno>> symbol_seqs_;

    // redis snap requester
    RedisSnapRequester    redis_snap_requester_;

    // sync snap and updater
    bool _update_seqno(const TExchange& exchange, const TSymbol& symbol, type_seqno sequence_no);
    bool _check_snap_received(const TExchange& exchange, const TSymbol& symbol) const;
    void _set(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    mutable std::mutex mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;
    
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;

    // 独立线程检查redis数据通道
    //std::mutex mutex_checker_;
    type_tick last_time_; // 上一次从redis收到行情的时间
    std::thread* checker_loop_ = nullptr;
    void _check_heartbeat();

    // 统计信息
    mutable std::mutex mutex_statistics_;
    type_tick last_statistic_time_; // 上一次计算统计信息时间
    type_tick last_nodata_time_; // 上一次检查交易所数据的时间
    unordered_map<TExchange, ExchangeStatistics> statistics_; // 各交易所统计信息
    void _update_statistics(const TExchange& exchange, const string& msg, const SDepthQuote& quote);
};