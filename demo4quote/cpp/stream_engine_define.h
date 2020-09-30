#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
using namespace std;

using TExchange = string;
using TSymbol = string;


#define PRICE_PRECISE 0.000000001
#define VOLUME_PRECISE 0.000000001
#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"

#define CALC_BASE(x) (int(pow(10, (x))))

struct SDecimal {
    long long Value;
    short Base;

    SDecimal() {
        Value = 0;
        Base = 0;
    }

    void From(const string& data) {
        std::string::size_type pos = data.find(".");
        Base = data.length() - pos - 1;
        Value = atof(data.c_str()) * CALC_BASE(Base);
    }

    double GetValue() const {
        return Value * 1.0 / CALC_BASE(Base);
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

inline void vassign(SDepthPrice& depth, const string& price, const double& volume) {
    depth.Price.From(price);    
    depth.Volume = volume;
};

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

    SMixQuote() {
        Asks = NULL;
        Bids = NULL;
    }
};
