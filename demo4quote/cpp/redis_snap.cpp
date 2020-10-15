#include "redis_snap.h"
#include "redis_quote.h"
#include "stream_engine_config.h"


void RedisSnapRequester::on_update_symbol(const string& exchange, const string& symbol) {
    
    string combinedSymbol = combine_symbol(exchange, symbol);

    std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
    if( symbols_.find(combinedSymbol) != symbols_.end() )
        return;
    //cout << "find new symbol:" << combinedSymbol << endl;
    symbols_[combinedSymbol] = 0;
    boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, exchange, symbol));
}

void RedisSnapRequester::_get_snap(const string& exchange, const string& symbol) {
    // 为线程池中每一个对象绑定一个redis_api对象
    std::thread::id thread_id = std::this_thread::get_id();
    RedisApiPtr redis_sync_api;
    auto iter = redis_sync_apis_.find(thread_id);
    if( iter == redis_sync_apis_.end() ) {
        redis_sync_api = RedisApiPtr{new utrade::pandora::CRedisApi{logger_}};
        redis_sync_api->RegisterRedis(host_, port_, password_, utrade::pandora::RM_GetData);
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
        unordered_map<string, int> tmp;
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
            // put all symbols to thread pool
            tmp = symbols_;
        }
        // 发送所有任务
        for (auto iter = tmp.begin(); iter != tmp.end(); ++iter) {
            string exchange, symbol;
            if( !split_symbol(iter->first, exchange, symbol) ) {
                continue;
            }
            boost::asio::post(boost::bind(&RedisSnapRequester::_get_snap, this, exchange, symbol));
        }

        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(CONFIG->quote_redis_snap_interval_));
    }

    pool.join();
}