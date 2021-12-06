#pragma once
#include "struct_define.h"
#include "interface_define.h"
#include "global_declare.h"

class TradeProcessor
{
public:

    void process(const TradeData&);

private:
    // callback
    QuoteSourceCallbackInterface *            engine_{nullptr};

    unordered_map<TSymbol, TradeData>         last_trade_map_;

    unordered_map<TSymbol, SMixerConfig>      config_map_;
};