#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/util/json.hpp"
#include "pandora/redis/redis_api.h"
using namespace std;
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include "redis_quote_define.h"

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

    void add_symbol(const TExchange& exchange, const TSymbol& symbol);

private:
    void _thread_loop();

    void _get_snap(const TExchange& exchange, const TSymbol& symbol);

private:
    // redis api对象
    RedisParams                params_;
    UTLogPtr                   logger_;
    unordered_map<std::thread::id, RedisApiPtr> redis_sync_apis_;

    // 回调对象
    RedisQuote*                quote_interface_ = nullptr;

    // 维护所有品种列表，作为请求全量的基础
    std::thread*               thread_loop_ = nullptr;// 请求线程
    std::mutex                 mutex_symbols_;
    unordered_set<TSymbol>     symbols_;
};