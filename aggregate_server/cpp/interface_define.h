#pragma once 

#include "struct_define.h"


class QuoteSourceInterface
{
public:
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool set_config(const TSymbol& symbol, const SSymbolConfig& config) { return true; };
};


class QuoteSourceCallbackInterface
{
public:
    // 行情接口
    virtual void on_snap( SDepthQuote& quote) = 0;

    // K线接口
    virtual void on_kline( KlineData& kline) = 0;

    // 交易接口
    virtual void on_trade( TradeData& trade) = 0;

    // 交易所异常无数据
    virtual void on_nodata_exchange(const TExchange& exchange){};
};