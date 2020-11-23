#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>
using namespace std;

#define CALC_BASE(x) (int(pow(10, (x))))

struct SDecimal {
    unsigned long long value;
    unsigned short base;

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