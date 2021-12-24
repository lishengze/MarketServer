#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"
#include "base/cpp/tinyformat.h"
#include "base/cpp/concurrentqueue.h"
#include "base/cpp/base_data_stuct.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "pandora/util/json.hpp"
#include "Log/log.h"

inline string make_symbolkey(const TExchange& exchange, const TSymbol& symbol) 
{
    return symbol + "." + exchange;
};

inline bool extract_symbolkey(const string& combined, TExchange& exchange, TSymbol& symbol) 
{
    std::string::size_type pos = combined.find(".");
    if( pos == std::string::npos) 
        return false;
    symbol = combined.substr(0, pos);
    exchange = combined.substr(pos+1);
    return true;
};

inline string make_redis_depth_key(const TExchange& exchange, const TSymbol& symbol) 
{
    return "DEPTHx|" + symbol + "." + exchange;
};

// 内部行情结构
struct SMixDepthPrice {
    SDecimal price;
    unordered_map<TExchange, SDecimal> volume;
    SMixDepthPrice* next;

    SMixDepthPrice() {
        next = NULL;
    }
};

struct SMixQuote {
    SMixDepthPrice* asks; // 卖盘
    SMixDepthPrice* bids; // 买盘
    SDecimal watermark;
    long long sequence_no;
    type_tick server_time;
    uint32 price_precise;
    uint32 volume_precise;

    SMixQuote() {
        asks = NULL;
        bids = NULL;
        price_precise = 0;
        volume_precise = 0;
    }

    unsigned int ask_length() const {
        return _get_length(asks);
    }

    unsigned int bid_length() const {
        return _get_length(bids);
    }

    unsigned int _get_length(const SMixDepthPrice* ptr) const {
        unsigned int ret = 0;
        while( ptr != NULL ) {
            ptr = ptr->next;
            ret ++;
        }
        return ret;
    }

    void release() {
        _release(asks);
        _release(bids);
        asks = NULL;
        bids = NULL;
    }

    void _release(SMixDepthPrice* ptr) {            
        SMixDepthPrice *tmp = ptr;
        while( tmp != NULL ) {
            // 删除             
            SMixDepthPrice* waitToDel = tmp;
            tmp = tmp->next;
            delete waitToDel;
        }
    }
};

struct SymbolFee
{
    int fee_type;       // 0表示fee不需要处理（默认），1表示fee值为比例，2表示fee值为绝对值
    double maker_fee;
    double taker_fee;

    SymbolFee() {
        fee_type = 0;
        maker_fee = taker_fee = 0.2;
    }

    bool operator==(const SymbolFee &rhs) const {
        return fee_type == rhs.fee_type && maker_fee == rhs.maker_fee && taker_fee == rhs.taker_fee;
    }

    void compute(const SDecimal& src, SDecimal& dst, bool is_ask) const
    {
        if( fee_type == 2 ) {
            if( is_ask ) {
                dst = src.add(maker_fee, true);
            } else {
                dst = src.add(taker_fee * (-1), false);
            }
        } else if( fee_type == 1 ) {
            if( is_ask ) {
                dst = src.multiple((100 + maker_fee) / 100.0, true);
            } else {
                dst = src.multiple((100 - taker_fee) / 100.0, false);
            }
        } else {
            dst = src;
        }
    }

    string str()
    {
        stringstream s_obj;

        s_obj << "fee_type: " << fee_type << " "
              << "maker_fee: " << maker_fee << " "
              << "taker_fee: " << taker_fee << " ";
        
        return s_obj.str();
    }
};

struct SNacosConfigByExchange
{
    SymbolFee fee;          // 手续费参数
    type_uint32 depth;      // 行情深度（暂时没用）
    type_uint32 precise;    // 价格精度
    type_uint32 vprecise;   // 成交量精度
    type_uint32 aprecise;   // 成交额精度（暂时没用）
    float frequency;        // 更新频率(每秒frequency次)

    string str() {
        stringstream s_obj;
        s_obj << "fee: " << fee.str() << " "
            << "depth: " << depth << " "
            << "precise: " << precise << " "
            << "vprecise: " << vprecise << " "
            << "aprecise: " << aprecise << " "
            << "frequency: " << frequency << " ";
        
        return s_obj.str();
    }

};

struct SNacosConfig
{
    type_uint32 depth;      // 【聚合】发布深度
    float frequency;        // 【聚合】更新频率（每秒frequency次）
    type_uint32 precise;    // 价格精度
    type_uint32 vprecise;   // 成交量精度
    type_uint32 aprecise;   // 成交额精度
    unordered_map<TExchange, SNacosConfigByExchange> exchanges;

    unordered_set<TExchange> get_exchanges() const {
        unordered_set<TExchange> ret;
        for( const auto& v : exchanges ) {
            ret.insert(v.first);
        }
        return ret;
    }

    string str()
    {
        stringstream s_obj;

        s_obj << "depth: " << depth << " "
              << "frequency: " << frequency << " "
              << "precise: " << precise << " "
              << "vprecise: " << vprecise << " "
              << "aprecise: " << vprecise << " \n";

        for (auto iter:exchanges)
        {
            s_obj << iter.first << " " << iter.second.str() << "\n";
        }

        return s_obj.str();
    }
};



using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;

struct RedisParams {
    string     host;
    int        port;
    string     password;
};

struct SExchangeConfig
{
    bool enable; // 是否启用
    int precise; // 价格精度
    int vprecise; // 成交量精度
    int aprecise; // 成交额精度
    float frequency; // 原始更新频率

    string str()
    {
        stringstream s_obj;
        s_obj << "enable " << enable << "\n"
              << "precise " << precise << "\n"
              << "vprecise " << vprecise << "\n"
              << "aprecise " << aprecise << "\n"
              << "frequency " << frequency << "\n";
        return s_obj.str();
    }

    bool operator==(const SExchangeConfig &rhs) const {
        return precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency;
    }

    string desc() const {
        return tfm::format("precise=%s vprecise=%s frequency=%s", ToString(precise), ToString(vprecise), ToString(frequency));
    }
};

using SSymbolConfig = unordered_map<TExchange, SExchangeConfig>;

struct SMixerConfig
{
    type_uint32 depth;
    type_uint32 precise;
    type_uint32 vprecise;
    type_uint32 aprecise;
    float frequency;
    unordered_map<TExchange, SymbolFee> fees;

    SMixerConfig() {

    }

    SMixerConfig(type_uint32 _depth, type_uint32 _precise, type_uint32 _vprecise, float _frequency, 
                            const unordered_map<TExchange, SNacosConfigByExchange>& _exchanges)
    {
        depth = _depth;
        precise = _precise;
        vprecise = _vprecise;
        frequency = _frequency;
        for( const auto& v : _exchanges ) 
        {
            fees[v.first] = v.second.fee;
        }        
    }

    string simple_str()
    {
        string result = "depth: " + std::to_string(depth) 
                        + ", precise: " + std::to_string(precise)
                        + ", vprecise: " + std::to_string(vprecise)
                        + ", aprecise: " + std::to_string(aprecise)
                        + ", fre: " + std::to_string(frequency);
        return result;
    }

    bool operator==(const SMixerConfig &rhs) const {
        return depth == rhs.depth && precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency&& fees == rhs.fees;
    }
    bool operator!=(const SMixerConfig &rhs) const {
        return !(*this == rhs);
    }
};
