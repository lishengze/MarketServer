#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <mutex>
using namespace std;
#include "config_updater.h"
#include "account_updater.h"

#define CALC_BASE(x) (int(pow(10, (x))))

struct SDecimal {
    unsigned long long Value;
    unsigned short Base;

    SDecimal() {
        Value = 0;
        Base = 0;
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

// 内部行情结构
#define MAX_EXCHANGE_LENGTH 10
#define MAX_DEPTH_LENGTH 20
#define MAX_SYMBOLNAME_LENGTH 32
#define MAX_EXCHANGENAME_LENGTH 32

struct SExchangeData {
    char name[MAX_EXCHANGENAME_LENGTH];
    double volume;

    SExchangeData() {
        strcpy(name, "");
        volume = 0;
    }
};

struct SInnerDepth {
    SDecimal price;
    SExchangeData exchanges[MAX_EXCHANGE_LENGTH];
    int exchange_length;

    SInnerDepth() {
        exchange_length = 0;
    }
};

struct SInnerQuote {
    char symbol[MAX_SYMBOLNAME_LENGTH];
    long long time;
    long long time_arrive;
    long long seq_no;
    SInnerDepth asks[MAX_DEPTH_LENGTH];
    int ask_length;
    SInnerDepth bids[MAX_DEPTH_LENGTH];
    int bid_length;

    SInnerQuote() {
        strcpy(symbol, "");
        time = 0;
        time_arrive = 0;
        seq_no = 0;
        ask_length = 0;
        bid_length = 0;
    }
};

class CallDataServeMarketStream;
class DataCenter {
public:
    DataCenter();
    ~DataCenter();

    void add_quote(const SInnerQuote& quote);
    void change_account(const AccountInfo& info);
    void change_configuration(const QuoteConfiguration& config);
    void add_client(CallDataServeMarketStream* client);
    void del_client(CallDataServeMarketStream* client);
private:
    mutable std::mutex                 mutex_datas_;
    unordered_map<string, SInnerQuote> datas_;
    
    mutable std::mutex                              mutex_clients_;
    unordered_map<CallDataServeMarketStream*, bool> clients_;
};