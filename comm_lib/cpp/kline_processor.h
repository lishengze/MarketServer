#pragma once

#include "base/cpp/base_data_stuct.h"
#include "interface_define.h"

#include "comm_log.h"

class KlineProcessor:public QuoteSourceCallbackInterface
{
public:

    KlineProcessor(QuoteSourceCallbackInterface * engine):engine_{engine} {}

    ~KlineProcessor();

    bool check_kline(const KlineData& kline);

    virtual void on_kline(KlineData& src);

private:
    QuoteSourceCallbackInterface *      engine_{nullptr};

    unordered_map<TSymbol, unordered_map<TExchange, KlineData>>         last_kline_map_;
};