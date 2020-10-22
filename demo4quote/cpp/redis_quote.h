#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"
using namespace std;
using njson = nlohmann::json;
#include "stream_engine_define.h"
#include "redis_snap.h"

bool parse_quote(const string& data, SDepthQuote& quote, bool isSnap, int precise);

class QuoteSourceInterface
{
public:
    virtual void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
    virtual void on_connected() = 0;
};

class RedisQuote : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisQuote(){};
    ~RedisQuote(){};

    // init
    void start(const RedisParams& params, UTLogPtr logger);
    void set_engine(QuoteSourceInterface* ptr) { engine_interface_ = ptr; }
    void subscribe(const string& channel);
    
    // callback from RedisSnapRequester
    void _on_snap(const string& exchange, const string& symbol, const string& data);


    // redis connect notify
    virtual void OnConnected();    
    // redis disconnect notify
    virtual void OnDisconnected(int status){ cout << "\n##### Redis MarketDispatcher::OnDisconnected ####\n" << endl; }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    // redis api
    RedisApiPtr     redis_api_;

    // redis snap requester
    RedisSnapRequester    redis_snap_requester_;

    // sync snap and updater
    bool _get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) const;
    std::mutex                             mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;
    
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;
};