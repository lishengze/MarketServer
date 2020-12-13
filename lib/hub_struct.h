#pragma once

#include "decimal.h"
#include <string>
using std::string;

#define DEPCH_LEVEL_COUNT 100

#pragma pack(1)
struct SDepthLevelData
{
    SDecimal price;
    SDecimal volume;

    SDepthLevelData() {
    }
};

const long UT_FID_SDepthData = 0x10000;
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
    static const long Fid = UT_FID_SDepthData;
};

const long UT_FID_KlineData = 0x10001;
struct KlineData
{
    KlineData(string symbol_str, type_tick time, double open, double high, 
                double low, double close, double volume_str):
                symbol{symbol_str}, index{time}, px_open{open}, px_close{close}, 
                px_high{high}, px_low{low}, volume{volume_str} {}
                    
    string symbol;
    string exchange;
    type_tick index;
    SDecimal px_open;
    SDecimal px_high;
    SDecimal px_low;
    SDecimal px_close;
    SDecimal volume;

    KlineData(){
        index = 0;
    }
    static const long Fid = UT_FID_KlineData;    
};

#pragma pack()

class HubCallback
{
public:
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { return 0; }

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { return 0; }
};
