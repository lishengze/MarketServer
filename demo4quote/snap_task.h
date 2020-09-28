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

class EngineInterface;
class SnapTaskCenter 
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    SnapTaskCenter() {
        thread_run_ = true;
    }

    ~SnapTaskCenter() {
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

    void on_update_symbol(const string& exchange, const string& symbol) {
        
        string combinedSymbol = combine_symbol(exchange, symbol);

        std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
        if( symbols_.find(combinedSymbol) != symbols_.end() )
            return;
        cout << "find new symbol:" << combinedSymbol << endl;
        symbols_[combinedSymbol] = 0;
        boost::asio::post(boost::bind(&SnapTaskCenter::get_snap, this, exchange, symbol));
    }

    void start(){
        thread_loop_ = new std::thread(&SnapTaskCenter::thread_loop, this);
    }

    void set_engine(EngineInterface* ptr) {
        engine_interface_ = ptr;
    }

private:
    void thread_loop(){
        
        boost::asio::thread_pool pool(4);

        while( thread_loop_ ) {
            // send all tasks 
            // todo
            unordered_map<string, int> tmp;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
                // put all symbols to thread pool
                tmp = symbols_;
            }
            string exchange, symbol;
            for (auto iter = tmp.begin(); iter != tmp.end(); ++iter) {
                if( !split_symbol(iter->first, exchange, symbol) ) {
                    continue;
                }
                boost::asio::post(boost::bind(&SnapTaskCenter::get_snap, this, exchange, symbol));
            }
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }

        pool.join();
    }

    void get_snap(const string& exchange, const string& symbol);

private:
    string host_;
    int port_;
    string password_;
    UTLogPtr logger_;
    unordered_map<std::thread::id, RedisApiPtr>                redis_sync_apis_;

    std::mutex                 mutex_symbols_;
    unordered_map<string, int> symbols_;

    EngineInterface*           engine_interface_;

    std::thread*               thread_loop_ = nullptr;
    std::atomic<bool>          thread_run_;
};