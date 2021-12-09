#pragma once

#include "base/cpp/decimal.h"
#include "base/cpp/quote.h"
#include "base/cpp/tinyformat.h"
#include "base/cpp/concurrentqueue.h"
#include <iostream>
#include <sstream>
#include <iomanip>

inline string make_symbolkey(const TExchange& exchange, const TSymbol& symbol) 
{
    return symbol + "." + exchange;
};

inline bool extract_symbolkey(const string& combined, TExchange& exchange, TSymbol& symbol) 
{
    std::string::size_type pos = combined.find(".");
    if( pos == std::string::npos) 
        return false;
    symbol = combined.substr(0, pos);
    exchange = combined.substr(pos+1);
    return true;
};

inline string make_redis_depth_key(const TExchange& exchange, const TSymbol& symbol) 
{
    return "DEPTHx|" + symbol + "." + exchange;
};

// 内部行情结构
struct SMixDepthPrice {
    SDecimal price;
    unordered_map<TExchange, SDecimal> volume;
    SMixDepthPrice* next;

    SMixDepthPrice() {
        next = NULL;
    }
};

struct SMixQuote {
    SMixDepthPrice* asks; // 卖盘
    SMixDepthPrice* bids; // 买盘
    SDecimal watermark;
    long long sequence_no;
    type_tick server_time;
    uint32 price_precise;
    uint32 volume_precise;

    SMixQuote() {
        asks = NULL;
        bids = NULL;
        price_precise = 0;
        volume_precise = 0;
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

    void release() {
        _release(asks);
        _release(bids);
        asks = NULL;
        bids = NULL;
    }

    void _release(SMixDepthPrice* ptr) {            
        SMixDepthPrice *tmp = ptr;
        while( tmp != NULL ) {
            // 删除             
            SMixDepthPrice* waitToDel = tmp;
            tmp = tmp->next;
            delete waitToDel;
        }
    }
};

struct SymbolFee
{
    int fee_type;       // 0表示fee不需要处理（默认），1表示fee值为比例，2表示fee值为绝对值
    double maker_fee;
    double taker_fee;

    SymbolFee() {
        fee_type = 0;
        maker_fee = taker_fee = 0.2;
    }

    bool operator==(const SymbolFee &rhs) const {
        return fee_type == rhs.fee_type && maker_fee == rhs.maker_fee && taker_fee == rhs.taker_fee;
    }

    void compute(const SDecimal& src, SDecimal& dst, bool is_ask) const
    {
        if( fee_type == 2 ) {
            if( is_ask ) {
                dst = src.add(maker_fee, true);
            } else {
                dst = src.add(taker_fee * (-1), false);
            }
        } else if( fee_type == 1 ) {
            if( is_ask ) {
                dst = src.multiple((100 + maker_fee) / 100.0, true);
            } else {
                dst = src.multiple((100 - taker_fee) / 100.0, false);
            }
        } else {
            dst = src;
        }
    }

    string str()
    {
        stringstream s_obj;

        s_obj << "fee_type: " << fee_type << " "
              << "maker_fee: " << maker_fee << " "
              << "taker_fee: " << taker_fee << " ";
        
        return s_obj.str();
    }
};

struct SNacosConfigByExchange
{
    SymbolFee fee;          // 手续费参数
    type_uint32 depth;      // 行情深度（暂时没用）
    type_uint32 precise;    // 价格精度
    type_uint32 vprecise;   // 成交量精度
    type_uint32 aprecise;   // 成交额精度（暂时没用）
    float frequency;        // 更新频率(每秒frequency次)

    string str() {
        stringstream s_obj;
        s_obj << "fee: " << fee.str() << " "
            << "depth: " << depth << " "
            << "precise: " << precise << " "
            << "vprecise: " << vprecise << " "
            << "aprecise: " << aprecise << " "
            << "frequency: " << frequency << " ";
        
        return s_obj.str();
    }

};

struct SNacosConfig
{
    type_uint32 depth;      // 【聚合】发布深度
    float frequency;        // 【聚合】更新频率（每秒frequency次）
    type_uint32 precise;    // 价格精度
    type_uint32 vprecise;   // 成交量精度
    type_uint32 aprecise;   // 成交额精度
    unordered_map<TExchange, SNacosConfigByExchange> exchanges;

    unordered_set<TExchange> get_exchanges() const {
        unordered_set<TExchange> ret;
        for( const auto& v : exchanges ) {
            ret.insert(v.first);
        }
        return ret;
    }

    string str()
    {
        stringstream s_obj;

        s_obj << "depth: " << depth << " "
              << "frequency: " << frequency << " "
              << "precise: " << precise << " "
              << "vprecise: " << vprecise << " "
              << "aprecise: " << vprecise << " \n";

        for (auto iter:exchanges)
        {
            s_obj << iter.first << " " << iter.second.str() << "\n";
        }

        return s_obj.str();
    }
};

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
    bool is_snap{false};

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

    SDepthQuote(const SDepthQuote&& src):
        raw_length{std::move(src.raw_length)},
        exchange{std::move(src.exchange)},
        symbol{std::move(src.symbol)},
        sequence_no{std::move(src.sequence_no)},
        origin_time{std::move(src.origin_time)},
        arrive_time{std::move(src.arrive_time)},
        server_time{std::move(src.server_time)},
        price_precise{std::move(src.price_precise)},
        volume_precise{std::move(src.volume_precise)},
        amount_precise{std::move(src.amount_precise)},
        asks{std::move(src.asks)},
        bids{std::move(src.bids)},
        is_snap{std::move(src.is_snap)}
    {

    }

    SDepthQuote(const SDepthQuote& src):
        raw_length{src.raw_length},
        exchange{src.exchange},
        symbol{src.symbol},
        sequence_no{src.sequence_no},
        origin_time{src.origin_time},
        arrive_time{src.arrive_time},
        server_time{src.server_time},
        price_precise{src.price_precise},
        volume_precise{src.volume_precise},
        amount_precise{src.amount_precise},
        asks{src.asks},
        bids{src.bids},
        is_snap{src.is_snap}
    {

    }

    SDepthQuote& operator = (const SDepthQuote& src)
    {
        if (this == &src) return *this;

        raw_length = src.raw_length;
        exchange = src.exchange;
        symbol = src.symbol;
        sequence_no = src.sequence_no;
        origin_time = src.origin_time;
        arrive_time = src.arrive_time;
        server_time = src.server_time;
        price_precise = src.price_precise;
        volume_precise = src.volume_precise;
        amount_precise = src.amount_precise;
        asks = src.asks;
        bids = src.bids;
        is_snap = src.is_snap;
        return *this;
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

struct TradeData
{
    type_tick   time{0};
    SDecimal    price;
    SDecimal    volume;
    TSymbol     symbol;
    TExchange   exchange;

    TradeData() {
        time = 0;
    }

    TradeData(const TradeData&& other):
    time{std::move(other.time)},
    price{std::move(other.price)},
    volume{std::move(other.volume)},
    symbol{std::move(other.symbol)},
    exchange{std::move(other.exchange)}
    {

    }

    TradeData(const TradeData& other):
    time{other.time},
    price{other.price},
    volume{other.volume},
    symbol{other.symbol},
    exchange{other.exchange}
    {

    }    

    TradeData& operator = (const TradeData& other)
    {
        if (this == &other) return *this;

        time = other.time;
        price= other.price;
        volume = other.volume;
        symbol = other.symbol;
        exchange = other.exchange;

        return *this;
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

struct SMixerConfig
{
    type_uint32 depth;
    type_uint32 precise;
    type_uint32 vprecise;
    type_uint32 aprecise;
    float frequency;
    unordered_map<TExchange, SymbolFee> fees;

    SMixerConfig() {

    }

    SMixerConfig(type_uint32 _depth, type_uint32 _precise, type_uint32 _vprecise, float _frequency, 
                            const unordered_map<TExchange, SNacosConfigByExchange>& _exchanges)
    {
        depth = _depth;
        precise = _precise;
        vprecise = _vprecise;
        frequency = _frequency;
        for( const auto& v : _exchanges ) 
        {
            fees[v.first] = v.second.fee;
        }        
    }

    bool operator==(const SMixerConfig &rhs) const {
        return depth == rhs.depth && precise == rhs.precise && vprecise == rhs.vprecise && frequency == rhs.frequency&& fees == rhs.fees;
    }
    bool operator!=(const SMixerConfig &rhs) const {
        return !(*this == rhs);
    }
};
