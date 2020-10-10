#include "redis_snap.h"
#include "redis_quote.h"
#include "stream_engine_config.h"

void RedisSnapRequester::get_snap(const string& exchange, const string& symbol) {
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
    
    string depth_key = make_redis_depth_key(exchange, symbol);
    string depthData = redis_sync_api->SyncGet(depth_key);
    UT_LOG_INFO(CONFIG->logger_, "get_snap: " << depthData);
    quote_interface_->__on_snap(exchange, symbol, depthData);
}