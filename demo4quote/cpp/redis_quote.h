#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"
using namespace std;
using njson = nlohmann::json;
#include "redis_quote_snap.h"

class QuoteSourceInterface
{
public:
    virtual void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_connected() = 0;
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
    void _on_snap(const TExchange& exchange, const TSymbol& symbol, const string& data);


    // redis connect notify
    virtual void OnConnected();    
    // redis disconnect notify
    virtual void OnDisconnected(int status){ cout << "\n##### Redis MarketDispatcher::OnDisconnected ####\n" << endl; }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    RedisParams params_;
    // redis api
    RedisApiPtr     redis_api_;
    // 市场序号
    unordered_map<TExchange, seq_no> exchange_seqs_;

    // redis snap requester
    RedisSnapRequester    redis_snap_requester_;

    // sync snap and updater
    bool _update_seqno(const TExchange& exchange, seq_no sequence_no);
    bool _check_snap_received(const TExchange& exchange, const TSymbol& symbol) const;
    void _set(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    mutable std::mutex mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;
    
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;

    // 独立线程检查redis数据通道
    //std::mutex mutex_checker_;
    long long last_time_;    
    std::thread* checker_loop_ = nullptr;
    void _check_heartbeat();
};