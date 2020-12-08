#pragma once

#include "hub_struct.h"
#include "pandora/util/thread_safe_singleton.hpp"

#include "updater_quote.h"
#include "update_kline.h"

#define HUB utrade::pandora::ThreadSafeSingleton<HubEntity>::DoubleCheckInstance()

class HubEntity final : public IQuoteUpdater, public IKlineUpdater
{
public:
    HubEntity();
    ~HubEntity();

    int start();

    int stop();

    int set_callback(HubCallback* callback) { callback_ = callback; return 0; }

    int get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);

    // IQuoteUpdater
    virtual void on_snap(const SEData& quote);

    // IKlineUpdater
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const KlineData& kline);
private:
    // 回调接口
    HubCallback* callback_;

    // 行情接入
    QuoteUpdater quote_updater_;

    // K线接入
    KlineUpdater kline_updater_;
};
