#pragma once

#include "base/cpp/base_data_stuct.h"
#include "comm_interface_define.h"

#include "comm_log.h"

COMM_NAMESPACE_START

class KlineProcessor:public QuoteSourceCallbackInterface
{
public:

    KlineProcessor(QuoteSourceCallbackInterface * engine):engine_{engine} {}

    ~KlineProcessor();

    bool check_kline(KlineData& kline);

    virtual void on_kline(KlineData& src);

private:
    QuoteSourceCallbackInterface *      engine_{nullptr};

    unordered_map<TSymbol, unordered_map<TExchange, KlineData>>         last_kline_map_;
};

DECLARE_PTR(KlineProcessor);

COMM_NAMESPACE_END