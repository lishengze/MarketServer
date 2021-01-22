#include "redis_quote_snap.h"
#include "redis_quote.h"
#include "stream_engine_config.h"


RedisSnapRequester::RedisSnapRequester() 
{
}

RedisSnapRequester::~RedisSnapRequester() 
{
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void RedisSnapRequester::init(const RedisParams& params, UTLogPtr logger, RedisQuote* callback)
{
    quote_interface_ = callback;
    if( redis_sync_api_ )
        return;
    redis_sync_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
    redis_sync_api_->RegisterRedis(params.host, params.port, params.password, utrade::pandora::RM_GetData);

}

void RedisSnapRequester::start()
{
    if( thread_loop_ )
        return;
    thread_loop_ = new std::thread(&RedisSnapRequester::_thread_loop, this);
}

void RedisSnapRequester::async_request_symbol(const TExchange& exchange, const TSymbol& symbol) 
{    
    string combinedSymbol = make_symbolkey(exchange, symbol);

    // 避免重复请求
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
        if( symbols_.find(combinedSymbol) != symbols_.end() )
            return;
        symbols_.insert(combinedSymbol);
    }

    // 添加请求任务
    _add_event(exchange, symbol, 1);
}

void RedisSnapRequester::_add_event(const TExchange& exchange, const TSymbol& symbol, int delay_seconds)
{
    EventData evt;
    evt.exchange = exchange;
    evt.symbol = symbol;
    evt.event_time = delay_seconds * 1000 + get_miliseconds();

    events_.enqueue(evt);
}

void RedisSnapRequester::_get_snap(const TExchange& exchange, const TSymbol& symbol) 
{    
    // 请求redis key
    string depth_key = make_redis_depth_key(exchange, symbol);
    _log_and_print("RedisSnapRequester: get snap %s", depth_key);
    string depthData = redis_sync_api_->SyncGet(depth_key);
    if( !quote_interface_->_on_snap(exchange, symbol, depthData) ) {
        _add_event(exchange, symbol, 1);
        return;
    }

    // 从symbols中去除
    std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
    string combinedSymbol = make_symbolkey(exchange, symbol);
    symbols_.erase(combinedSymbol);
}

void RedisSnapRequester::_thread_loop()
{
    // 初始化线程池
    //boost::asio::thread_pool pool(4);

    while( thread_loop_ ) 
    {
        EventData evt;
        while( events_.try_dequeue(evt) ) {
            while( true ) {                
                type_tick now = get_miliseconds();
                //tfm::printfln("%u %u", evt.event_time, now);
                if( evt.event_time <= now )
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(evt.event_time - now));
            }

            // 改为同步调用
            //boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, evt.exchange, evt.symbol));                
            _get_snap(evt.exchange, evt.symbol);
        }
        // 休眠
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    //pool.join();
}