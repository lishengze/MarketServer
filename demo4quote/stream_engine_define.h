#pragma once

#include <string>
#include <map>
#include <unordered_map>
using namespace std;

using TExchange = string;
using TSymbol = string;


#define PRICE_PRECISE 0.000000001
#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"

struct SDecimal {
    int Value;
    short Base;

    SDecimal() {
        memset(this, 0, sizeof(SDecimal));
    }

    void From(const string& data) {
        std::string::size_type pos = data.find(".");
        Base = data.length() - pos;
        Value = atof(data.c_str()) * (10^Base);
    }

    double GetValue() const {
        return Value * 1.0 / (10^Base);
    }

    bool operator <(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value < d.Value * (10^(Base-d.Base));
        } else {
            return Value * (10^(d.Base-Base)) < d.Value;
        }
    }
    bool operator >(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value > d.Value * (10^(Base-d.Base));
        } else {
            return Value * (10^(d.Base-Base)) > d.Value;
        }
    }
    bool operator ==(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value == d.Value * (10^(Base-d.Base));
        } else {
            return Value * (10^(d.Base-Base)) == d.Value;
        }
    }
    bool operator <=(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value <= d.Value * (10^(Base-d.Base));
        } else {
            return Value * (10^(d.Base-Base)) <= d.Value;
        }
    }
    bool operator >=(const SDecimal& d) const {
        if( Base > d.Base ) {
            return Value >= d.Value * (10^(Base-d.Base));
        } else {
            return Value * (10^(d.Base-Base)) >= d.Value;
        }
    }

    SDecimal operator + (const SDecimal &d) const {
        SDecimal ret;
        if( Base > d.Base ) {
            ret.Base = Base;
            ret.Value = Value + d.Value * (10^(Base-d.Base));
        } else {
            ret.Base = d.Base;
            ret.Value = Value * (10^(d.Base-Base)) + d.Value;
        }
        return ret;
    }
    SDecimal operator - (const SDecimal &d) const {
        SDecimal ret;
        if( Base > d.Base ) {
            ret.Base = Base;
            ret.Value = Value - d.Value * (10^(Base-d.Base));
        } else {
            ret.Base = d.Base;
            ret.Value = Value * (10^(d.Base-Base)) - d.Value;
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
        memset(this, 0, sizeof(SDepthPrice));
    }
};

#define MAX_DEPTH 50
struct SDepthQuote {
    char Exchange[32];
    char Symbol[32];
    long long SequenceNo;
    char TimeArrive[64];
    SDepthPrice Asks[MAX_DEPTH];
    int AskLength;
    SDepthPrice Bids[MAX_DEPTH];
    int BidLength;

    SDepthQuote() {
        memset(this, 0, sizeof(SDepthQuote));
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