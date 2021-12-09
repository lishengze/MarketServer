#pragma once
#include "struct_define.h"
#include "interface_define.h"
#include "global_declare.h"

class TradeProcessor
{
public:

    TradeProcessor(QuoteSourceCallbackInterface* engine):engine_{engine}
    { }

    void process(TradeData& src);

    void set_config(unordered_map<TSymbol, SMixerConfig>& new_config)
    {
        symbol_config_ = new_config;
    }

private:
    // callback
    QuoteSourceCallbackInterface *            engine_{nullptr};

    unordered_map<TSymbol, TradeData>         last_trade_map_;

    unordered_map<TSymbol, SMixerConfig>      symbol_config_;
};