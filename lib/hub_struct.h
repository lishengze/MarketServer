#pragma once

#include "decimal.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include "common_datatype_define.h"
#include "quark/cxx/assign.h"

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
    SDepthData(const SDepthData& other)
    {
        assign(symbol, other.symbol);
        assign(exchange, other.exchange);
        assign(tick, other.tick);
        assign(seqno, other.seqno);
        assign(ask_length, other.ask_length);
        assign(bid_length, other.bid_length);
    
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

        for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
        {
            asks[i] = other.asks[i];
            bids[i] = other.bids[i];
        }
        return *this;
    }
    
    symbol_type symbol;
    symbol_type exchange;
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
                    
    symbol_type symbol;
    symbol_type exchange;
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
};

using KlineDataPtr = boost::shared_ptr<KlineData>;


struct Trade
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
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { return 0; }

    // 成交（推送）
    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade) { return 0; }

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { return 0; }
};
