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

inline bool split_symbol(const string& combined, string& exchange, string& symbol) {
    std::string::size_type pos = combined.find(".");
    if( pos == std::string::npos) 
        return false;
    symbol = combined.substr(0, pos);
    exchange = combined.substr(pos+1);
    return true;
};

class RedisQuote;
class RedisSnapRequester 
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    RedisSnapRequester() {}

    ~RedisSnapRequester() {
        if (thread_loop_) {
            if (thread_loop_->joinable()) {
                thread_loop_->join();
            }
            delete thread_loop_;
        }
    }

    // init
    void init(const string& host, const int& port, const string& password, UTLogPtr logger){
        host_ = host;
        port_ = port;
        password_ = password;
        logger_ = logger;
    }

    void start(){
        thread_loop_ = new std::thread(&RedisSnapRequester::_thread_loop, this);
    }

    void set_engine(RedisQuote* ptr) { quote_interface_ = ptr; }

    void on_update_symbol(const string& exchange, const string& symbol);

private:
    void _thread_loop();

    void _get_snap(const string& exchange, const string& symbol);

private:
    string host_;
    int port_;
    string password_;
    UTLogPtr logger_;
    unordered_map<std::thread::id, RedisApiPtr> redis_sync_apis_;

    std::mutex                 mutex_symbols_;
    unordered_map<string, int> symbols_;

    RedisQuote*                quote_interface_;

    std::thread*               thread_loop_ = nullptr;
};