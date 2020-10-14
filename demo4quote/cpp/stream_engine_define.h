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
    unsigned long long Value;
    short Base;

    SDecimal() {
        Value = 0;
        Base = 0;
    }

    static SDecimal MaxDecimal() {
        SDecimal ret;
        ret.Value = ULLONG_MAX;
        ret.Base = 0;
        return ret;
    }

    static SDecimal MinDecimal() {
        SDecimal ret;
        return ret;
    }

    void From(const string& data, int precise = -1, bool ceiling = false) {
        std::string::size_type pos = data.find(".");
        Base = data.length() - pos - 1;
        if( precise >= 0 && precise < Base ) { // 精度调整
            Base = precise;
            string newData = data.substr(0, pos + 1 + precise);
            Value = atof(newData.c_str()) * CALC_BASE(Base);
            if( ceiling )
                Value += 1;
        } else {
            Value = atof(data.c_str()) * CALC_BASE(Base);
        }
    }

    void From(const SDecimal& data, int precise = -1, bool ceiling = false) {
        if( precise == -1 || data.Base <= precise ) {
            Value = data.Value;
            Base = data.Base;
            return;
        }

        if( ceiling ) {
            Value = ceil(data.Value / CALC_BASE(data.Base - precise));
        } else {
            Value = floor(data.Value / CALC_BASE(data.Base - precise));
        }
        Base = precise;
    }

    double GetValue() const {
        return Value * 1.0 / CALC_BASE(Base);
    }

    string GetStrValue() const {
        char precise[25];
        sprintf(precise, "%d", Base+1);

        char holder[1024];
        string fmt = "%0" + string(precise) + "lld";
        sprintf(holder, fmt.c_str(), Value);
        string ret = holder;
        ret.insert(ret.begin() + ret.length() - Base, '.');
        return ret;
    }

    bool operator <(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value < d.Value * CALC_BASE(Base-d.Base);
        } else {
            return Value * CALC_BASE(d.Base-Base) < d.Value;
        }
    }
    bool operator >(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value > d.Value * CALC_BASE(Base-d.Base);
        } else {
            return Value * CALC_BASE(d.Base-Base) > d.Value;
        }
    }
    bool operator ==(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value == d.Value * CALC_BASE(Base-d.Base);
        } else {
            return Value * CALC_BASE(d.Base-Base) == d.Value;
        }
    }
    bool operator <=(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value <= d.Value * CALC_BASE(Base-d.Base);
        } else {
            return Value * CALC_BASE(d.Base-Base) <= d.Value;
        }
    }
    bool operator >=(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value >= d.Value * CALC_BASE(Base-d.Base);
        } else {
            return Value * CALC_BASE(d.Base-Base) >= d.Value;
        }
    }

    SDecimal operator + (const SDecimal &d) const {
        SDecimal ret;
        if( Base > d.Base ) {
            ret.Base = Base;
            ret.Value = Value + d.Value * CALC_BASE(Base-d.Base);
        } else {
            ret.Base = d.Base;
            ret.Value = Value * CALC_BASE(d.Base-Base) + d.Value;
        }
        return ret;
    }
    SDecimal operator - (const SDecimal &d) const {
        SDecimal ret;
        if( Base > d.Base ) {
            ret.Base = Base;
            ret.Value = Value - d.Value * CALC_BASE(Base-d.Base);
        } else {
            ret.Base = d.Base;
            ret.Value = Value * CALC_BASE(d.Base-Base) - d.Value;
        }
        return ret;
    }
    SDecimal operator / (const int &d) const {
        SDecimal ret;
        ret.Base = Base;
        ret.Value = Value / d;
        return ret;
    }
};

struct SDepthPrice {
    SDecimal Price;
    double Volume;

    SDepthPrice() {
        Volume = 0;
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

#define MAX_DEPTH 50
struct SDepthQuote {
    char Exchange[32];
    char Symbol[32];
    long long SequenceNo;
    char TimeArrive[64];
    SDepthPrice Asks[MAX_DEPTH];        // 卖盘
    int AskLength;
    SDepthPrice Bids[MAX_DEPTH];        // 买盘
    int BidLength;

    SDepthQuote() {
        vassign(Exchange, "");
        vassign(Symbol, "");
        vassign(SequenceNo, 0);
        vassign(TimeArrive, "");
        AskLength = 0;
        BidLength = 0;
    }
};

struct SMixDepthPrice {
    SDecimal Price;
    unordered_map<TExchange, double> Volume;
    SMixDepthPrice* Next;

    SMixDepthPrice() {
        Next = NULL;
    }
};

#define MAX_MIXDEPTH 50
struct SMixQuote {
    SMixDepthPrice* Asks; // 卖盘
    SMixDepthPrice* Bids; // 买盘
    SDecimal Watermark;
    long long SequenceNo;

    SMixQuote() {
        Asks = NULL;
        Bids = NULL;
    }
};

using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;

inline long long get_miliseconds() {
    auto time_now = chrono::system_clock::now();
	auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}
