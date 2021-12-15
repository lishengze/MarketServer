#pragma once 

#include "base/cpp/base_data_stuct.h"

#include "comm_log.h"


class QuoteSourceCallbackInterface
{
public:
    // 行情接口
    virtual void on_snap( SDepthQuote& quote) { };

    // K线接口
    virtual void on_kline( KlineData& kline) { };

    // 交易接口
    virtual void on_trade( TradeData& trade) { };

    // 交易所异常无数据
    virtual void on_nodata_exchange(const TExchange& exchange){};
};