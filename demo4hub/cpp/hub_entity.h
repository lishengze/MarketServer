#pragma once

#include "hub_struct.h"
#include "pandora/util/singleton.hpp"
#include "updater_quote.h"

#define HUB utrade::pandora::Singleton<HubEntity>::GetInstance()

class HubEntity final : public IQuoteUpdater
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
private:
    // 回调接口
    HubCallback* callback_;

    // 行情接入
    QuoteUpdater quote_updater_;
};
