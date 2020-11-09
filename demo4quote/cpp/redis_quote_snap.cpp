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

void RedisSnapRequester::add_symbol(const TExchange& exchange, const TSymbol& symbol) {
    
    string combinedSymbol = make_symbolkey(exchange, symbol);
    std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
    if( symbols_.find(combinedSymbol) != symbols_.end() )
        return;
    symbols_.insert(combinedSymbol);
    boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, exchange, symbol));
}

void RedisSnapRequester::reset_symbol() 
{    
    std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
    symbols_.clear();
}

void RedisSnapRequester::_get_snap(const TExchange& exchange, const TSymbol& symbol) {
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
    string depthData = redis_sync_api->SyncGet(depth_key);
    quote_interface_->_on_snap(exchange, symbol, depthData);
}

void RedisSnapRequester::_thread_loop(){
    // 初始化线程池
    boost::asio::thread_pool pool(4);

    while( thread_loop_ ) {
        // 获取所有任务
        unordered_set<string> tmp;
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
            //tmp = symbols_;
        }
        // 发送所有任务
        for (auto iter = tmp.begin(); iter != tmp.end(); ++iter) {
            string exchange, symbol;
            if( !extract_symbolkey(*iter, exchange, symbol) ) {
                continue;
            }
            boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, exchange, symbol));
        }

        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(CONFIG->quote_redis_snap_interval_));
    }

    pool.join();
}