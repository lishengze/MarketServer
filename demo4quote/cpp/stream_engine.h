#pragma once

#include "redis_quote.h"
#include "redis_quote_replay.h"
#include "quote_mixer2.h"
#include "quote_dumper.h"
#include "grpc_server.h"
#include "nacos_client.h"
#include "kline_mixer.h"
#include "kline_database.h"

#define STREAMENGINE utrade::pandora::Singleton<StreamEngine>::GetInstance()

class StreamEngine : public QuoteSourceInterface, public INacosCallback
{
public:
    StreamEngine();
    ~StreamEngine();

    void start();

    // from QuoteSourceInterface
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_nodata_exchange(const TSymbol& symbol);
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);
    //void on_precise_changed(const TSymbol& symbol, int precise);

    // from INacosCallback
    void on_symbol_channged(const NacosString& symbols);

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);

private:
    // redis quote replay
    RedisQuoteReplay quote_replay_;
    // redis quote upstream
    RedisQuote quote_source_;
    // mix quotation
    //QuoteMixer quote_mixer_;
    // mix quotation version2(current)
    QuoteMixer2 quote_mixer2_;
    // single symbol quotation
    //QuoteSingle quote_single_;
    // quotation dumper
    QuoteDumper quote_dumper_;
    // nacos client
    NacosClient nacos_client_;

    // 聚合K线
    // 缓存短期K线，计算聚合结果
    KlineMixer kline_mixer_;

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