#pragma once

#include "hub_struct.h"

class HubInterface
{
public:
    static int start(string file_name);
    static int stop();
    static int set_callback(HubCallback* callback);

    // K线数据（请求）
    static int get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);

    // 最近交易（请求）
    static int get_lasttrades(vector<Trade>& trades);
};
