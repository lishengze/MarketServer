#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"
#include "base/cpp/tinyformat.h"
#include "base/cpp/concurrentqueue.h"
#include <iostream>
#include <sstream>
#include <iomanip>
struct SDepth {
    SDecimal volume;    // 单量
    unordered_map<TExchange, SDecimal> volume_by_exchanges; // 聚合行情才有用
};

struct SDepthQuote {
    type_uint32 raw_length; // 原始包大小（用来比较压缩效率）
    string exchange;        // 交易所
    string symbol;          // 代码
    type_seqno sequence_no; // 序号
    type_tick origin_time;  // 交易所时间  单位微妙
    type_tick arrive_time;  // 服务器收到时间 单位毫秒
    type_tick server_time;  // 服务器处理完成时间 单位毫秒
    uint32 price_precise;   // 价格精度（来自配置中心）
    uint32 volume_precise;  // 成交量精度（来自配置中心）
    uint32 amount_precise;  // 成交额精度（来自配置中心）
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

    std::string str() const
    {
        std::stringstream s_obj;
        s_obj << "exchange: " << exchange << ","
                << "symbol: " << symbol << ", "
                << "asks.size: " << asks.size()  << ","
                << "bids.size: " << bids.size()  << ""
            << "\n";
        return s_obj.str();        
    }

    std::string basic_str()
    {
        std::stringstream s_obj;
        s_obj << std::setw(16) << "raw_length: " << std::setw(16) << raw_length << "\n"
            << std::setw(16) << "exchange: " << std::setw(16) << exchange << "\n"
            << std::setw(16) << "symbol: " << std::setw(16) << symbol << "\n"
            << std::setw(16) << "sequence_no: " << std::setw(16) << sequence_no << "\n"
            << std::setw(16) << "arrive_time: " << std::setw(16) << arrive_time << "\n"
            << std::setw(16) << "server_time: " << std::setw(16) << server_time << "\n"
            << std::setw(16) << "price_precise: " << std::setw(16) << price_precise << "\n"
            << std::setw(16) << "amount_precise: " << std::setw(16) << amount_precise << "\n"
            << std::setw(16) << "asks.size: " << std::setw(16) << asks.size()  << "\n"
            << std::setw(16) << "bids.size: " << std::setw(16) << bids.size()  << "\n"
            << "\n";
        return s_obj.str();
    }



    std::string depth_str()
    {
        std::stringstream s_obj;
        s_obj << "asks detail: \n";
        for (auto iter:asks)
        {
            s_obj << std::setw(16) << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";
        }
        s_obj << "bids detail: \n";
        for (auto iter:bids)
        {
            s_obj << std::setw(16) << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";
        }        
        return s_obj.str();
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
#define SNAP_HEAD "__SNAPx" // 为了统一接口，自定义的消息头

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

    string exchange;
    string symbol;
    int    resolution;

    KlineData(){
        index = 0;
        volume = 0;
    }

    void print_debug() const {
        tfm::printf("index=%lu, open=%s, high=%s, low=%s, close=%s, vol=%s", index, px_open.get_str_value(), px_high.get_str_value(), px_low.get_str_value(), 
        px_close.get_str_value(), volume.get_str_value());
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

struct SExchangeConfig
{
    bool enable; // 是否启用
    int precise; // 价格精度
    int vprecise; // 成交量精度
    int aprecise; // 成交额精度
    float frequency; // 原始更新频率

    string str()
    {
        stringstream s_obj;
        s_obj << "enable " << enable << "\n"
              << "precise " << precise << "\n"
              << "vprecise " << vprecise << "\n"
              << "aprecise " << aprecise << "\n"
              << "frequency " << frequency << "\n";
        return s_obj.str();
    }

    bool operator==(const SExchangeConfig &rhs) const {
        return precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency;
    }

    string desc() const {
        return tfm::format("precise=%s vprecise=%s frequency=%s", ToString(precise), ToString(vprecise), ToString(frequency));
    }
};
using SSymbolConfig = unordered_map<TExchange, SExchangeConfig>;
class QuoteSourceInterface
{
public:
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool set_config(const TSymbol& symbol, const SSymbolConfig& config) { return true; };
};

class QuoteSourceCallbackInterface
{
public:
    // 行情接口
    virtual void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;
    virtual void on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) = 0;

    // K线接口
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init) = 0;

    // 交易接口
    virtual void on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade) = 0;

    // 交易所异常无数据
    virtual void on_nodata_exchange(const TExchange& exchange){};

    virtual void erase_dead_exchange_symbol_depth(const TExchange& exchange, const TSymbol& symbol) { }
};