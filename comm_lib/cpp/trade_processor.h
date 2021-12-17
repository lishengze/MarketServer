#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"
#include "comm_declare.h"
#include "interface_define.h"

#include "comm_log.h"

COMM_NAMESPACE_START

class TradeProcessor:public QuoteSourceCallbackInterface
{
public:

    TradeProcessor(QuoteSourceCallbackInterface* engine):engine_{engine}
    { }

    bool check(TradeData& trade);

    virtual void on_trade( TradeData& trade);

private:
    // callback
    QuoteSourceCallbackInterface *                                      engine_{nullptr};

    unordered_map<TSymbol, unordered_map<TExchange, TradeData>>         last_trade_map_;
};

DECLARE_PTR(TradeProcessor);

COMM_NAMESPACE_END