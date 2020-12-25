#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"

struct SDepthQuote {
    type_uint32 raw_length;
    string exchange;
    string symbol;
    type_seqno sequence_no;
    type_tick arrive_time;
    map<SDecimal, SDecimal> asks; // 买盘
    map<SDecimal, SDecimal> bids; // 卖盘

    SDepthQuote() {
        raw_length = 0;
        exchange = "";
        symbol = "";
        sequence_no = 0;
    }

    void print() const {
        for( const auto&v : asks ) {
            cout << "asks\t" << v.first.get_str_value() << "\t" << v.second.get_str_value() << "\t" << v.second.data_.real_.value_ << endl;
        }
        for( const auto&v : bids ) {
            cout << "bids\t" << v.first.get_str_value() << "\t" << v.second.get_str_value() << "\t" << v.second.data_.real_.value_ << endl;
        }
    }
};

using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
#define TRADE_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"
#define KLINE_1MIN_HEAD "KLINEx|"
#define KLINE_60MIN_HEAD "SLOW_KLINEx|"

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

struct Trade
{
    type_tick time;
    SDecimal price;
    SDecimal volume;

    Trade() {
        time = 0;
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
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init) = 0;

    // 交易接口
    virtual void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade) = 0;
};