#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"
using namespace std;
using njson = nlohmann::json;
#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"

#define MIN1_KLINE_PREFIX "KLINEx|"

#pragma pack(1)
struct KlineData
{
    type_tick index;
    SDecimal px_open;
    SDecimal px_high;
    SDecimal px_low;
    SDecimal px_close;
    double volume;

    KlineData(){
        index = 0;
        volume = 0;
    }
};
#pragma pack()

struct RedisParams {
    string     host;
    int        port;
    string     password;
};

class IRedisKlineSource {
public:
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline) = 0;
};


class RedisKlineSource : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisKlineSource();
    ~RedisKlineSource();

    void start();
    void register_callback(IRedisKlineSource* ptr) { callbacks_.insert(ptr); }

    // redis connect notify
    virtual void OnConnected();    
    // redis disconnect notify
    virtual void OnDisconnected(int status);
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg);

private:
    // 上一次连接时间
    type_tick last_redis_time_;
    // redis接口对象
    RedisApiPtr     redis_api_;
    
    // callback
    set<IRedisKlineSource*> callbacks_;

    // 上一次从redis收到行情的时间
    type_tick last_time_; 
};