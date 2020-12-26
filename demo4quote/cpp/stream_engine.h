#pragma once

#include "redis_quote.h"
#include "redis_quote_replay.h"
#include "quote_mixer2.h"
#include "quote_dumper.h"
#include "grpc_server.h"
#include "kline_mixer.h"
#include "kline_database.h"
#include "nacos_client.h"

class StreamEngine : public QuoteSourceInterface, public INacosCallback
{
public:
    StreamEngine();
    ~StreamEngine();

    void start();

    // from QuoteSourceInterface
    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);
    void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);
    void on_nodata_exchange(const TSymbol& symbol);
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init);
    void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade);

    // from INacosCallback
    void on_config_channged(const NacosString& symbols);

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);

private:
    // redis quote replay
    RedisQuoteReplay quote_replay_;

    // quotation dumper
    QuoteDumper quote_dumper_;

    // redis quote upstream
    RedisQuote quote_source_;

    QuoteCacher quote_cacher_;
    
    // mix quotation
    QuoteMixer2 quote_mixer2_;

    // nacos client
    NacosClient nacos_client_;

    // 聚合K线，K线缓存
    // 缓存短期K线，计算聚合结果
    KlineMixer kline_mixer_;

    KlineHubber kline_hubber_;

    // 数据库功能实现
    // 入数据库接口
    // 从数据库查询接口
    KlineDatabase kline_db_;
    
    // 服务
    // grpc模式，stream推送最新1分钟，5分钟，60分钟和日数据
    ServerEndpoint server_endpoint_;

    // 动态配置信息
    std::unordered_map<TSymbol, SNacosConfig> symbols_;
};