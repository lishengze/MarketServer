#pragma once

#include "redis_quote.h"
#include "redis_snap.h"
#include "quote_mixer.h"
#include "quote_dumper.h"

class StreamEngine : public QuoteInterface 
{
public:
    using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    StreamEngine();
    ~StreamEngine();

    void start();

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);

private:
    bool _get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) const;

    // origin market data
    std::mutex                             mutex_markets_;
    unordered_map<TExchange, TMarketQuote> markets_;

    // redis quote upstream
    RedisQuote *redis_quote_;

    // mix quotation
    QuoteMixer *quote_mixer_;
    QuoteDumper *quote_dumper_;
};