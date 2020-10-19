#pragma once

#include "redis_quote.h"
#include "quote_mixer.h"
#include "quote_mixer2.h"
#include "quote_single.h"
#include "quote_dumper.h"
#include "grpc_server.h"

class StreamEngine : public QuoteInterface 
{
public:
    StreamEngine();
    ~StreamEngine();

    void start();

    // from QuoteInterface
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_connected();

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);

private:

    // redis quote upstream
    RedisQuote redis_quote_;

    // mix quotation
    QuoteMixer quote_mixer_;
    QuoteMixer2 quote_mixer2_;
    QuoteSingle quote_single_;
    QuoteDumper quote_dumper_;
};