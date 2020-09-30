#pragma once

#include "redis_hub.h"
#include "snap_task.h"
#include "quote_mixer.h"
#include "quote_dumper.h"

class EngineInterface
{
public:
    virtual void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) = 0;
};


class StreamEngine : public EngineInterface 
{
public:
    using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    StreamEngine(){
        UTLogPtr logger = utrade::pandora::UTLog::getStrategyLogger("SnapTaskCenter", "SnapTaskCenter");

        // request snap 
        snap_task_.init("45.249.244.59", 6666, "rkqFB4,wpoMmHqT6he}r", logger);
        snap_task_.set_engine(this);

        redis_hub_.init("45.249.244.59", 6666, "rkqFB4,wpoMmHqT6he}r", logger);
        redis_hub_.set_engine(this);
        //redis_hub_.SubscribeAll();
        //redis_hub_.Subscribe("EOS_USDT", "XDAEX");
        //redis_hub_.SubscribeByExchange(exchangeToSubscribe);
    }

    ~StreamEngine(){}

    void start() {
        snap_task_.start();
        redis_hub_.start();
    }

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
        //cout << "on_snap exchange: " << exchange << " symbol: " << symbol << "\n" << endl;

        std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
        markets_[exchange][symbol] = quote;
        quote_mixer_.on_mix_snap(exchange, symbol, quote);
    };

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
        //cout << "on_update exchange: " << exchange << " symbol: " << symbol << "\n" << endl;
        // update new symbol
        snap_task_.on_update_symbol(exchange, symbol);

        std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
        SDepthQuote lastQuote;
        if( !_get_quote(exchange, symbol, lastQuote) )
            return;
        // filter by SequenceNo
        if( quote.SequenceNo < lastQuote.SequenceNo )
            return;
        quote_mixer_.on_mix_update(exchange, symbol, quote);
    };

private:
    bool _get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) {
        auto iter = markets_.find(exchange);
        if( iter == markets_.end() )
            return false;
        const TMarketQuote& marketQuote = iter->second;
        auto iter2 = marketQuote.find(symbol);
        if( iter2 == marketQuote.end() )
            return false;
        quote = iter2->second;
        return true;
    }

    std::mutex                              mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;

    // symbol center
    SnapTaskCenter    snap_task_;

    // redis upstream
    RedisHub   redis_hub_;

    // mix quotation
    QuoteMixer  quote_mixer_;
    //QuoteDumper  quote_mixer_;
};