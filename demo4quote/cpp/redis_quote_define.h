#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"

template<class T,class S>
inline void vassign(T &r, S v)
{
	r = v;
}

template<class T>
inline void vassign(T &r, const T v)
{
	r = v;
}

inline void vassign(char * r, unsigned int len, const char *v)
{
    unsigned int l = std::min(len-1, (unsigned int)strlen(v));
	strncpy(r, v, l);
    r[l] = '\0';
}

inline void vassign(char * r, unsigned int len, const std::string &v)
{
    unsigned int l = std::min(len-1, (unsigned int)v.length());
    strncpy(r, v.c_str(), l);
    r[l] = '\0';
}
struct SDepthQuote {
    type_uint32 raw_length;
    string exchange;
    string symbol;
    type_seqno sequence_no;
    type_tick arrive_time;
    map<SDecimal, double> asks; // 买盘
    map<SDecimal, double> bids; // 卖盘

    SDepthQuote() {
        raw_length = 0;
        exchange = "";
        symbol = "";
        sequence_no = 0;
    }
};

using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"
#define KLINE_1MIN_HEAD "KLINEx|"

inline string make_symbolkey(const TExchange& exchange, const TSymbol& symbol) {
    return symbol + "." + exchange;
};

inline bool extract_symbolkey(const string& combined, TExchange& exchange, TSymbol& symbol) {
    std::string::size_type pos = combined.find(".");
    if( pos == std::string::npos) 
        return false;
    symbol = combined.substr(0, pos);
    exchange = combined.substr(pos+1);
    return true;
};

inline string make_redis_depth_key(const TExchange& exchange, const TSymbol& symbol) {
    return "DEPTHx|" + symbol + "." + exchange;
};

#pragma pack(1)
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
        volume = 0;
    }
};
#pragma pack()

struct RedisParams {
    string     host;
    int        port;
    string     password;
};

class QuoteSourceInterface
{
public:
    // 行情接口
    virtual void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_nodata_exchange(const TExchange& exchange){};

    // K线接口
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline) = 0;
};