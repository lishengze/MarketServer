#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/util/json.hpp"
#include "pandora/redis/redis_api.h"
using namespace std;
using json = nlohmann::json;
#include "stream_engine_define.h"

bool parse_snap(const string& data, SDepthQuote& quote);

inline string make_redis_depth_key(const string& exchange, const string& symbol) {
    return "DEPTHx|" + symbol + "." + exchange;
};

#define SUBMODE_ALL 1
#define SUBMODE_CHANNEL 2
#define SUBMODE_PATTERN 3

class EngineInterface;
class RedisHub : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisHub():engine_interface_(NULL){};
    ~RedisHub(){};

    // init
    void init(const string& host, const int& port, const string& password, UTLogPtr logger){
        redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
        // register callback
        redis_api_->RegisterSpi(this);
        // redis connector
        redis_api_->RegisterRedis(host, port, password, utrade::pandora::RM_Subscribe);
    }

    void set_engine(EngineInterface* ptr) {
        engine_interface_ = ptr;
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
        sub_mode_ = SUBMODE_ALL;
        redis_api_->PSubscribeTopic("*");
    }


    // redis connect notify
    virtual void OnConnected(){
        cout << "\n##### Redis MarketDispatcher::OnConnected ####\n" << endl;
        if( sub_mode_ == SUBMODE_ALL ) {
            redis_api_->PSubscribeTopic("*");
        } else {

        }
    }
    // redis disconnect notify
    virtual void OnDisconnected(int status){
        cout << "\n##### Redis MarketDispatcher::OnDisconnected ####\n" << endl;
    }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    int             sub_mode_;
    RedisApiPtr     redis_api_;
    EngineInterface *engine_interface_;
};