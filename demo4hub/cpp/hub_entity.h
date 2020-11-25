#pragma once

#include "hub_struct.h"
#include "pandora/util/singleton.hpp"

#define HUB utrade::pandora::Singleton<HubEntity>::GetInstance()

class HubEntity final 
{
public:
    HubEntity();
    ~HubEntity();

    int start();

    int stop();

    int set_callback(HubCallback* callback) { callback_ = callback; return 0; }

    int get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);

private:
    HubCallback* callback_;
};
