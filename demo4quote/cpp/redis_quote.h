#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/util/json.hpp"
#include "pandora/redis/redis_api.h"
using namespace std;
using json = nlohmann::json;
#include "stream_engine_define.h"
#include "redis_snap.h"

bool parse_snap(const string& data, SDepthQuote& quote, bool isSnap);

inline string make_redis_depth_key(const string& exchange, const string& symbol) {
    return "DEPTHx|" + symbol + "." + exchange;
};

class QuoteInterface
{
public:
    virtual void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
};

class RedisQuote : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisQuote():engine_interface_(NULL){};
    ~RedisQuote(){};

    // init
    void init(const string& host, const int& port, const string& password, UTLogPtr logger){
        redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
        redis_api_->RegisterSpi(this);
        redis_api_->RegisterRedis(host, port, password, utrade::pandora::RM_Subscribe);
        
        // request snap 
        redis_snap_.init(host, port, password, logger);
    }

    void set_engine(QuoteInterface* ptr) {
        engine_interface_ = ptr;
        redis_snap_.set_engine(ptr);
    }

    // lauch the market data
    void start() {
    }


    // subscribe
    void SubscribeByExchange(const string& exchangeName) {
        redis_api_->PSubscribeTopic("UPDATEx|*." + exchangeName);
    }

    void Subscribe(const string& symbol, const string& exchange) {
        redis_api_->SubscribeTopic("UPDATEx|" + symbol + "." + exchange);
    }

    void SubscribeAll() {
        redis_api_->PSubscribeTopic("*");
    }


    // redis connect notify
    virtual void OnConnected();
    
    // redis disconnect notify
    virtual void OnDisconnected(int status){
        cout << "\n##### Redis MarketDispatcher::OnDisconnected ####\n" << endl;
    }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    RedisApiPtr     redis_api_;

    QuoteInterface *engine_interface_;

    GetRedisSnap    redis_snap_;
};