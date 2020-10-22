#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <chrono>
using namespace std;

using TExchange = string;
using TSymbol = string;

#define PRICE_PRECISE 0.000000001
#define VOLUME_PRECISE 0.000000001
#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"

#define CALC_BASE(x) (pow(10, (x)))

struct SDecimal {
    unsigned long long value;
    short base;

    SDecimal() {
        value = 0;
        base = 0;
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

inline void vassign(char * r, char *v)
{
	strcpy(r,v);
}

inline void vassign(char * r,const char *v)
{
	strcpy(r,v);
}

inline void vassign(char * r,const std::string &v)
{
    strcpy(r,v.c_str());
}

// redis行情二进制结构
struct SDepthPrice {
    SDecimal price;
    double volume;

    SDepthPrice() {
        volume = 0;
    }
};

#define MAX_DEPTH 100
struct SDepthQuote {
    char exchange[32];
    char symbol[32];
    long long sequence_no;
    char time_arrive[64];
    SDepthPrice asks[MAX_DEPTH];        // 卖盘
    unsigned int ask_length;
    SDepthPrice bids[MAX_DEPTH];        // 买盘
    unsigned int bid_length;

    SDepthQuote() {
        vassign(exchange, "");
        vassign(symbol, "");
        vassign(sequence_no, 0);
        vassign(time_arrive, "");
        ask_length = 0;
        bid_length = 0;
    }
};

// 内部行情结构（链表）
struct SMixDepthPrice {
    SDecimal price;
    unordered_map<TExchange, double> volume;
    SMixDepthPrice* next;

    SMixDepthPrice() {
        next = NULL;
    }
};

#define MAX_MIXDEPTH 100
struct SMixQuote {
    SMixDepthPrice* asks; // 卖盘
    SMixDepthPrice* bids; // 买盘
    SDecimal watermark;
    long long sequence_no;

    SMixQuote() {
        asks = NULL;
        bids = NULL;
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
};

using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;

inline long long get_miliseconds() {
    auto time_now = chrono::system_clock::now();
	auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}
