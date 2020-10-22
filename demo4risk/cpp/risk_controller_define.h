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
using namespace std;

#define CALC_BASE(x) (int(pow(10, (x))))

struct SDecimal {
    unsigned long long Value;
    unsigned short Base;

    SDecimal() {
        Value = 0;
        Base = 0;
    }

    static SDecimal max_decimal() {
        SDecimal ret;
        ret.Value = ULLONG_MAX;
        ret.Base = 0;
        return ret;
    }

    static SDecimal min_decimal() {
        SDecimal ret;
        return ret;
    }

    void from(const string& data, int precise = -1, bool ceiling = false) {
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

    void from(const SDecimal& data, int precise = -1, bool ceiling = false) {
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

    double get_value() const {
        return Value * 1.0 / CALC_BASE(Base);
    }

    string get_str_value() const {
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
    SDecimal operator / (const double &d) const {
        SDecimal ret;
        ret.Base = Base;
        ret.Value = Value / d;
        return ret;
    }
    SDecimal operator * (const double &d) const {
        SDecimal ret;
        ret.Base = Base;
        ret.Value = Value * d;
        return ret;
    }
};
#define MAX_EXCHANGE_LENGTH 10
#define MAX_DEPTH_LENGTH 100
#define MAX_SYMBOLNAME_LENGTH 32
#define MAX_EXCHANGENAME_LENGTH 32
#define TSymbol string
#define TExchange string