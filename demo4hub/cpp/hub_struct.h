#pragma once

#include "decimal.h"

#define DEPCH_LEVEL_COUNT 100

#pragma pack(1)
struct SDepthLevelData
{
    SDecimal price;
    double volume;

    SDepthLevelData() {
        volume = 0;
    }
};

struct SDepthData
{
    string symbol;
    string exchange;
    type_tick tick;
    type_seqno seqno;
    SDepthLevelData asks[DEPCH_LEVEL_COUNT];
    type_length ask_length;
    SDepthLevelData bids[DEPCH_LEVEL_COUNT];
    type_length bid_length;
    
    SDepthData() {
        tick = 0;
        seqno = 0;
        ask_length = 0;
        bid_length = 0;
    }
};

struct KlineData
{
    type_tick index;
    SDecimal px_open;
    SDecimal px_high;
    SDecimal px_low;
    SDecimal px_close;
    SDecimal volume;

    KlineData(){
        index = 0;
    }
};

#pragma pack()

class HubCallback
{
public:
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { return 0; }

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const KlineData& kline) { return 0; }
};
