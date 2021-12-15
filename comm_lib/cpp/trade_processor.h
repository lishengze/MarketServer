#pragma once
#include "base/cpp/base_data_stuct.h"
#include "interface_define.h"

#include "comm_log.h"

class TradeProcessor:public QuoteSourceCallbackInterface
{
public:

    TradeProcessor(QuoteSourceCallbackInterface* engine):engine_{engine}
    { }

    bool check(const TradeData& trade);

    virtual void on_trade( TradeData& trade) { };

private:
    // callback
    QuoteSourceCallbackInterface *                                      engine_{nullptr};

    unordered_map<TSymbol, unordered_map<TExchange, TradeData>>         last_trade_map_;
};