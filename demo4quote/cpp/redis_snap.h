#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/util/json.hpp"
#include "pandora/redis/redis_api.h"
#include "stream_engine_define.h"
using namespace std;
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>


inline string combine_symbol(const string& exchange, const string& symbol) {
    return symbol + "." + exchange;
};

inline bool decombine_symbol(const string& combined, string& exchange, string& symbol) {
    std::string::size_type pos = combined.find(".");
    if( pos == std::string::npos) 
        return false;
    symbol = combined.substr(0, pos);
    exchange = combined.substr(pos+1);
    return true;
};

inline string make_redis_depth_key(const string& exchange, const string& symbol) {
    return "DEPTHx|" + symbol + "." + exchange;
};

struct RedisParams {
    string     host;
    int        port;
    string     password;
};
class RedisQuote;
class RedisSnapRequester 
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisSnapRequester();
    ~RedisSnapRequester();

    void init(const RedisParams& params, UTLogPtr logger){
        params_ = params;
        logger_ = logger;
    }

    void set_engine(RedisQuote* ptr) { quote_interface_ = ptr; }

    void start();

    void on_update_symbol(const string& exchange, const string& symbol);

private:
    void _thread_loop();

    void _get_snap(const string& exchange, const string& symbol);

private:
    RedisParams                params_;
    UTLogPtr                   logger_;
    unordered_map<std::thread::id, RedisApiPtr> redis_sync_apis_;

    // 回调对象
    RedisQuote*                quote_interface_ = nullptr;

    // 维护所有品种列表，作为请求全量的基础
    std::thread*               thread_loop_ = nullptr;// 请求线程
    std::mutex                 mutex_symbols_;
    unordered_map<string, int> symbols_;
};