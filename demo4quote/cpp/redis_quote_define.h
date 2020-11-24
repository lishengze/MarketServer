#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>
using namespace std;


// SDecimal
#define CALC_BASE(x) (pow(10, (x)))

#define _println_(...)                                             \
    do {                                                           \
        char content[1024];                                        \
        sprintf(content, __VA_ARGS__);                             \
        std::cout << content << std::endl;                         \
    } while(0)                                                     

struct SDecimal {
    unsigned long long value;
    short base;

    SDecimal() {
        value = 0;
        base = 0;
    }

    SDecimal(double v, double bias = 0.00001) {
        from(v, bias);
    }

    static SDecimal max_decimal() {
        SDecimal ret;
        ret.value = ULLONG_MAX;
        ret.base = 0;
        return ret;
    }

    static SDecimal min_decimal() {
        SDecimal ret;
        return ret;
    }

    void from(double v, double bias = 0.00001) {
        int count = 0, limit = 100;
        while( count >= limit && abs(get_value() - v)/v > bias ) {
            base += 1;
            count += 1;
            value = v * CALC_BASE(base);
        }
    }

    void from(const string& data, int precise = -1, bool ceiling = false) {
        std::string::size_type pos = data.find(".");
        base = data.length() - pos - 1;
        if( precise >= 0 && precise < base ) { // 精度调整
            base = precise;
            string newData = data.substr(0, pos + 1 + precise);
            value = atof(newData.c_str()) * CALC_BASE(base);
            if( ceiling )
                value += 1;
        } else {
            value = atof(data.c_str()) * CALC_BASE(base);
        }
    }

    void from(const SDecimal& data, int precise = -1, bool ceiling = false) {
        if( precise == -1 || data.base <= precise ) {
            value = data.value;
            base = data.base;
            return;
        }

        if( ceiling ) {
            value = ceil(data.value / CALC_BASE(data.base - precise));
        } else {
            value = floor(data.value / CALC_BASE(data.base - precise));
        }
        base = precise;
    }

    double get_value() const {
        return value * 1.0 / CALC_BASE(base);
    }

    string get_str_value() const {
        char precise[25];
        sprintf(precise, "%d", base+1);

        char holder[1024];
        string fmt = "%0" + string(precise) + "lld";
        sprintf(holder, fmt.c_str(), value);
        string ret = holder;
        ret.insert(ret.begin() + ret.length() - base, '.');
        return ret;
    }

    bool operator <(const SDecimal& d) const {
        if( base > d.base ) {
            return value < d.value * CALC_BASE(base-d.base);
        } else {
            return value * CALC_BASE(d.base-base) < d.value;
        }
    }
    bool operator >(const SDecimal& d) const {
        if( base > d.base ) {
            return value > d.value * CALC_BASE(base-d.base);
        } else {
            return value * CALC_BASE(d.base-base) > d.value;
        }
    }
    bool operator ==(const SDecimal& d) const {
        if( base > d.base ) {
            return value == d.value * CALC_BASE(base-d.base);
        } else {
            return value * CALC_BASE(d.base-base) == d.value;
        }
    }
    bool operator <=(const SDecimal& d) const {
        if( base > d.base ) {
            return value <= d.value * CALC_BASE(base-d.base);
        } else {
            return value * CALC_BASE(d.base-base) <= d.value;
        }
    }
    bool operator >=(const SDecimal& d) const {
        if( base > d.base ) {
            return value >= d.value * CALC_BASE(base-d.base);
        } else {
            return value * CALC_BASE(d.base-base) >= d.value;
        }
    }

    SDecimal operator + (const SDecimal &d) const {
        SDecimal ret;
        if( base > d.base ) {
            ret.base = base;
            ret.value = value + d.value * CALC_BASE(base-d.base);
        } else {
            ret.base = d.base;
            ret.value = value * CALC_BASE(d.base-base) + d.value;
        }
        return ret;
    }
    SDecimal operator - (const SDecimal &d) const {
        SDecimal ret;
        if( base > d.base ) {
            ret.base = base;
            ret.value = value - d.value * CALC_BASE(base-d.base);
        } else {
            ret.base = d.base;
            ret.value = value * CALC_BASE(d.base-base) - d.value;
        }
        return ret;
    }
    SDecimal operator / (const double &d) const {
        SDecimal ret;
        ret.base = base;
        ret.value = value / d;
        return ret;
    }
    SDecimal operator * (const double &d) const {
        SDecimal ret;
        ret.base = base;
        ret.value = value * d;
        return ret;
    }
};

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

// redis行情二进制结构
struct SDepthPrice {
    SDecimal price;         // 处理后价格
    double volume;

    SDepthPrice() {
        volume = 0;
    }
};

#define MAX_DEPTH 200
#define MAX_EXCHANGE_NAME_LENGTH 32
#define MAX_SYMBOL_NAME_LENGTH 32
using type_seqno = unsigned long;

struct SDepthQuote {
    int raw_length;
    char exchange[MAX_EXCHANGE_NAME_LENGTH];
    char symbol[MAX_SYMBOL_NAME_LENGTH];
    type_seqno sequence_no;
    //char time_arrive[64];
    SDepthPrice asks[MAX_DEPTH];        // 卖盘
    unsigned int ask_length;
    SDepthPrice bids[MAX_DEPTH];        // 买盘
    unsigned int bid_length;

    SDepthQuote() {
        vassign(exchange, MAX_EXCHANGE_NAME_LENGTH, "");
        vassign(symbol, MAX_SYMBOL_NAME_LENGTH, "");
        vassign(sequence_no, 0);
        //vassign(time_arrive, "");
        ask_length = 0;
        bid_length = 0;
        raw_length = 0;
    }
};

using TExchange = string;
using TSymbol = string;
using TSymbolKey = string;
using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
using type_tick = int64_t;

#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"

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

struct RedisParams {
    string     host;
    int        port;
    string     password;
};

class QuoteSourceInterface
{
public:
    virtual void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_connected(){};
    virtual void on_disconnected(){};
    virtual void on_nodata_exchange(const TExchange& exchange){};
};