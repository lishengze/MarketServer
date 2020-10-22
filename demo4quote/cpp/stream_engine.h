#pragma once

#include "redis_quote.h"
#include "quote_mixer.h"
#include "quote_mixer2.h"
#include "quote_single.h"
#include "quote_dumper.h"
#include "grpc_server.h"

class StreamEngine : public QuoteSourceInterface 
{
public:
    StreamEngine();
    ~StreamEngine();

    void start();

    // from QuoteSourceInterface
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);
    void on_connected();

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);

private:

    // redis quote upstream
    RedisQuote quote_source_;
    // mix quotation
    QuoteMixer quote_mixer_;
    // mix quotation version2(current)
    QuoteMixer2 quote_mixer2_;
    // single symbol quotation
    QuoteSingle quote_single_;
    // quotation dumper
    QuoteDumper quote_dumper_;
};