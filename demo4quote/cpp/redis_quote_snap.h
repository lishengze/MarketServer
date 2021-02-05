#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "redis_quote_define.h"

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

class RedisQuote;
class RedisSnapRequester 
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisSnapRequester();
    ~RedisSnapRequester();

    void init(const RedisParams& params, UTLogPtr logger, RedisQuote* callback);
    
    void start();

    void async_request_symbol(const TExchange& exchange, const TSymbol& symbol);

private:
    void _thread_loop();

    void _get_snap(const TExchange& exchange, const TSymbol& symbol);

private:
    // redis api对象
    RedisApiPtr         redis_sync_api_ = nullptr;

    // 回调对象
    RedisQuote*         quote_interface_ = nullptr;
    
    // 请求线程
    std::thread*        thread_loop_ = nullptr;
    std::atomic<bool>   thread_run_;

    // 维护所有品种列表，作为请求全量的基础
    mutable std::mutex         mutex_symbols_;
    unordered_set<TSymbol>     symbols_;
    
    // 触发事件的队列
    struct EventData{
        TExchange exchange;
        TSymbol symbol;
        type_tick event_time;
    };    
    moodycamel::ConcurrentQueue<EventData>            events_;
    void _add_event(const TExchange& exchange, const TSymbol& symbol, int delay_seconds);
};