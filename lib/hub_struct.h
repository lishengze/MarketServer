#pragma once

#include "decimal.h"
#include "quote.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include "common_datatype_define.h"
#include "quark/cxx/assign.h"
#include "pandora/package/package_simple.h"

using std::string;

#define DEPCH_LEVEL_COUNT 200

#pragma pack(1)
struct SDepthLevelData
{
    SDecimal price;
    SDecimal volume;

    SDepthLevelData() {
    }
};

const long UT_FID_SDepthData = 10000;
struct SDepthData:virtual public PacakgeBaseData
{
    SDepthData(const SDepthData& other)
    {
        assign(symbol, other.symbol);
        assign(exchange, other.exchange);
        assign(tick, other.tick);
        assign(seqno, other.seqno);
        assign(ask_length, other.ask_length);
        assign(bid_length, other.bid_length);
        assign(is_raw, other.is_raw);
    
        // cout << "SDepthData: " << symbol << " src symbol: " << other.symbol << endl;

        for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
        {
            asks[i] = other.asks[i];
            bids[i] = other.bids[i];
        }
    }

    SDepthData & operator =(const SDepthData& other) 
    {
        // cout << "SDepthData & operator = " << endl;

        assign(symbol, other.symbol);
        assign(exchange, other.exchange);
        assign(tick, other.tick);
        assign(seqno, other.seqno);
        assign(ask_length, other.ask_length);
        assign(bid_length, other.bid_length);
        assign(is_raw, other.is_raw);

        for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
        {
            asks[i] = other.asks[i];
            bids[i] = other.bids[i];
        }
        return *this;
    }
    
    symbol_type symbol;
    symbol_type exchange;
    type_tick tick;     // 来自交易所的时间戳
    type_tick tick1;    // 来自streamengine收到行情的本地时间戳
    type_tick tick2;    // 来自streamengine发出数据包的时间戳
    type_tick tick3;    // 来自riskcontrol发出的数据包的时间戳
    type_seqno seqno;
    SDepthLevelData asks[DEPCH_LEVEL_COUNT];
    type_length ask_length;
    SDepthLevelData bids[DEPCH_LEVEL_COUNT];
    type_length bid_length;

    bool is_raw{false};
    
    SDepthData() {
        tick = 0;
        seqno = 0;
        ask_length = 0;
        bid_length = 0;
    }
    static const long Fid = UT_FID_SDepthData;
};
using SDepthDataPtr = boost::shared_ptr<SDepthData>;

const long UT_FID_KlineData = 10001;
struct KlineData:virtual public PacakgeBaseData
{
    KlineData(string symbol_str, type_tick time, double open, double high, 
                double low, double close, double volume_str)
    {
        assign(symbol, symbol_str);
        assign(index, time);
        assign(px_open, SDecimal(open));
        assign(px_close, SDecimal(close));
        assign(px_high, SDecimal(high));
        assign(px_low, SDecimal(low));
        assign(volume, SDecimal(volume));        
    }                

    KlineData(const KlineData& other)
    {
        assign(symbol, other.symbol);
        assign(exchange, other.exchange);
        assign(index, other.index);
        assign(px_open, other.px_open);
        assign(px_close, other.px_close);
        assign(px_high, other.px_high);
        assign(px_low, other.px_low);
        assign(volume, other.volume);
    }

    void reset(const KlineData& other)
    {
        assign(symbol, other.symbol);
        assign(exchange, other.exchange);
        assign(index, other.index);
        assign(px_open, other.px_open);
        assign(px_close, other.px_close);
        assign(px_high, other.px_high);
        assign(px_low, other.px_low);
        assign(volume, other.volume);

        clear_ = false;     
    }

    void clear() { clear_ = true;}

    bool is_clear() {return clear_;}    
                    
    symbol_type symbol{NULL};
    symbol_type exchange{NULL};
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

    bool        clear_{false};
    int         frequency_{60};
};

using KlineDataPtr = boost::shared_ptr<KlineData>;

struct Trade:virtual public PacakgeBaseData
{
    symbol_type symbol;
    symbol_type exchange;
    type_tick time;
    SDecimal price;
    SDecimal volume;

    Trade() {
        time = 0;
    }
};

#pragma pack()

class HubCallback
{
public:
    // 原始深度数据推送
    virtual int on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth) { return 0; }

    // 风控后深度数据
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { return 0; }

    // 成交
    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade) { return 0; }

    // K线数据
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { return 0; }
};
