#include "redis_quote_snap.h"
#include "redis_quote.h"
#include "stream_engine_config.h"


RedisSnapRequester::RedisSnapRequester() {
}

RedisSnapRequester::~RedisSnapRequester() {
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void RedisSnapRequester::start(){
    thread_loop_ = new std::thread(&RedisSnapRequester::_thread_loop, this);
}

void RedisSnapRequester::add_symbol(const TExchange& exchange, const TSymbol& symbol) 
{    
    string combinedSymbol = make_symbolkey(exchange, symbol);
    std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
    if( symbols_.find(combinedSymbol) != symbols_.end() )
        return;
    symbols_.insert(combinedSymbol);
    _add_event(exchange, symbol, 1);
}

void RedisSnapRequester::_add_event(const TExchange& exchange, const TSymbol& symbol, int seconds)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_events_ };
    EventData evt;
    evt.exchange = exchange;
    evt.symbol = symbol;
    evt.event_time = seconds * 1000 + get_miliseconds();
    events_.push_back(evt);
}

void RedisSnapRequester::_get_snap(const TExchange& exchange, const TSymbol& symbol) 
{
    // 为线程池中每一个对象绑定一个redis_api对象
    std::thread::id thread_id = std::this_thread::get_id();
    RedisApiPtr redis_sync_api;
    auto iter = redis_sync_apis_.find(thread_id);
    if( iter == redis_sync_apis_.end() ) {
        redis_sync_api = RedisApiPtr{new utrade::pandora::CRedisApi{logger_}};
        redis_sync_api->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_GetData);
        redis_sync_apis_[thread_id] = redis_sync_api;
    } else {
        redis_sync_api = iter->second;
    }
    
    // 请求redis key
    string depth_key = make_redis_depth_key(exchange, symbol);
    _log_and_print("RedisSnapRequester: get snap %s", depth_key.c_str());
    string depthData = redis_sync_api->SyncGet(depth_key);
    if( !quote_interface_->_on_snap(exchange, symbol, depthData) ) {
        _add_event(exchange, symbol, 1);
    } else {        
        std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
        string combinedSymbol = make_symbolkey(exchange, symbol);
        symbols_.erase(combinedSymbol);
    }
}

void RedisSnapRequester::_thread_loop()
{
    // 初始化线程池
    boost::asio::thread_pool pool(4);

    vector<EventData> evts;
    while( thread_loop_ ) 
    {
        type_tick now = get_miliseconds();
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_events_ };
            while( events_.size() > 0 ) {
                EventData& evt = events_.front();
                //cout << evt.event_time << " " << now << endl;
                if( evt.event_time <= now ) {
                    evts.push_back(evt);
                    events_.pop_front();
                } else {
                    break;
                }
            }
        }

        if( evts.size() > 0 ) {
            // 发送所有任务
            for (const auto& v : evts) {
                boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, v.exchange, v.symbol));
            }
            evts.clear();
        }

        // 休眠
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }


    pool.join();
}