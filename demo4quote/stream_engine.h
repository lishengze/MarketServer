#pragma once

#include "redis_hub.h"
#include "snap_task.h"
#include "stream_engine_define.h"

class EngineInterface
{
public:
    virtual void on_snap(const string& exchange, const string& symbol) = 0;
    virtual void on_update(const string& exchange, const string& symbol) = 0;
};


class StreamEngine : public EngineInterface 
{
public:
    using TMarketQuotePtr = boost::shared_ptr<unordered_map<TSymbol, string>>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    StreamEngine(){
        UTLogPtr logger = utrade::pandora::UTLog::getStrategyLogger("SnapTaskCenter", "SnapTaskCenter");

        // request snap 
        snap_task_.init("45.249.244.59", 6666, "rkqFB4,wpoMmHqT6he}r", logger);
        snap_task_.set_engine(this);

        redis_hub_.init("45.249.244.59", 6666, "rkqFB4,wpoMmHqT6he}r", logger);
        redis_hub_.set_engine(this);
        redis_hub_.SubscribeAll();
        //redis_hub_.Subscribe("EOS_USDT", "XDAEX");
        //redis_hub_.SubscribeByExchange(exchangeToSubscribe);
    }

    ~StreamEngine(){}

    void start() {
        snap_task_.start();
        redis_hub_.start();
    }

    void on_snap(const string& exchange, const string& symbol){
        cout << "on_snap exchange: " << exchange << " symbol: " << symbol << "\n" << endl;
    };

    void on_update(const string& exchange, const string& symbol){        
        cout << "on_update exchange: " << exchange << " symbol: " << symbol << "\n" << endl;
        // update new symbol
        snap_task_.on_update_symbol(exchange, symbol);
    };

private:
    std::mutex                              mutex_markets_;
    unordered_map<TExchange, TMarketQuotePtr> markets_;

    // symbol center
    SnapTaskCenter    snap_task_;

    // redis upstream
    RedisHub   redis_hub_;
};