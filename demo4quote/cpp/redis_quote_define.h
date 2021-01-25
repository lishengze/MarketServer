#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"
#include "base/cpp/tinyformat.h"
#include "base/cpp/concurrentqueue.h"

struct SDepth {
    SDecimal volume;    // 单量
    unordered_map<TExchange, SDecimal> volume_by_exchanges; // 聚合行情才有用
};

struct SDepthQuote {
    type_uint32 raw_length; // 原始包大小（用来比较压缩效率）
    string exchange;        // 交易所
    string symbol;          // 代码
    type_seqno sequence_no; // 序号
    type_tick origin_time;  // 交易所时间
    type_tick arrive_time;  // 服务器收到时间
    type_tick server_time;  // 服务器处理完成时间
    uint32 price_precise;   // 价格精度（来自配置中心）
    uint32 volume_precise;  // 成交量精度（来自配置中心）
    map<SDecimal, SDepth> asks; // 买盘
    map<SDecimal, SDepth> bids; // 卖盘

    SDepthQuote() {
        raw_length = 0;
        exchange = "";
        symbol = "";
        sequence_no = 0;
        arrive_time = 0;
        server_time = 0;
        price_precise = 0;
        volume_precise = 0;
    }

    void print() const {
        for( const auto&v : asks ) {
            tfm::printf("[ask] %s:%s", v.first.get_str_value(), v.second.volume.get_str_value());
        }
        for( const auto&v : bids ) {
            tfm::printf("[bid] %s:%s", v.first.get_str_value(), v.second.volume.get_str_value());
        }
    }
};

using TMarketQuote = unordered_map<TSymbol, SDepthQuote>;
#define TRADE_HEAD "TRADEx"
#define DEPTH_UPDATE_HEAD "UPDATEx"
#define GET_DEPTH_HEAD "DEPTHx"
#define KLINE_1MIN_HEAD "KLINEx"
#define KLINE_60MIN_HEAD "SLOW_KLINEx"

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

struct SExchangeConfig
{
    int precise; // 交易所原始价格精度
    int vprecise; // 交易所原始成交量精度
    float frequency; // 原始更新频率

    bool operator==(const SExchangeConfig &rhs) const {
        return precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency;
    }

    string desc() const {
        return tfm::format("precise=%s vprecise=%s frequency=%s", ToString(precise), ToString(vprecise), ToString(frequency));
    }
};