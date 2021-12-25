#pragma once

#include "basic.h"
#include "decimal.h"
#include "pandora/util/json.hpp"
#include "pandora/util/float_util.h"
#include "pandora/util/time_util.h"

inline string get_sec_time_str(unsigned long time)
{
    return utrade::pandora::ToSecondStr(time * NANOSECONDS_PER_SECOND, "%Y-%m-%d %H:%M:%S");
}

struct SDepth {
    SDecimal volume;    // 单量
    unordered_map<TExchange, SDecimal> volume_by_exchanges; // 聚合行情才有用

    void mix_exchanges(const SDepth& src, double bias, uint32 kind=1) 
    {
        if (kind == 1 && bias > -100)
        {
            for( const auto& v : src.volume_by_exchanges ) 
            {                
                volume_by_exchanges[v.first] += (v.second * (1 + bias)) > 0 ? (v.second * (1 + bias)) : 0;
            }
        }
        else if (kind == 2)
        {
            for( const auto& v : src.volume_by_exchanges ) 
            {                
                volume_by_exchanges[v.first] += (v.second + bias) > 0 ? (v.second + bias) :0;
            }
        }



        volume = 0;
        for( const auto& v : volume_by_exchanges ) {
            volume += v.second;
        }
    }

    void set_total_volume()
    {
        volume = 0;
        for( const auto& v : volume_by_exchanges ) {
            volume += v.second;
        }        
    }    
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

    void get_asks(vector<pair<SDecimal, SDepth>>& depths) const {
        depths.clear();
        for( auto iter = asks.begin() ; iter != asks.end() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }

    void get_bids(vector<pair<SDecimal, SDepth>>& depths) const {
        depths.clear();
        for( auto iter = bids.rbegin() ; iter != bids.rend() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }
    
    string meta_str() const
    {
        try
        {
            std::stringstream stream_obj;

            stream_obj  << "depth_" 
                        << "Symbol_" << symbol
                        << "_Exchange_" << exchange;

            return stream_obj.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return "";
    }

    std::string str() const
    {
        std::stringstream s_obj;
        s_obj << "exchange: " << exchange << ","
                << "symbol: " << symbol << ", "
                << "seno " << sequence_no << ","
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


    void set_depth_json(nlohmann::json& ask_json, const map<SDecimal, SDepth>& depth)
    {
        try
        {
            for (auto iter:depth)
            {
                ask_json[iter.first.get_str_value()] = iter.second.volume.get_value();
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }        
    }

    string get_json_str()
    {
        try
        {
            nlohmann::json json_data;            

            json_data["Symbol"] = symbol;
            json_data["Exchange"] = exchange;
            json_data["Type"] = "snap";
            json_data["TimeArrive"] = origin_time;
            json_data["Msg_seq_symbol"] = sequence_no;

            nlohmann::json ask_json;
            nlohmann::json bid_json;

            set_depth_json(ask_json, asks);
            set_depth_json(bid_json, bids);

            json_data["AskDepth"] = ask_json;
            json_data["BidDepth"] = bid_json;

            return json_data.dump();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        return "";
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

    string meta_str() const
    {
        try
        {
            std::stringstream stream_obj;

            stream_obj  << "kline_" 
                        << "Symbol_" << symbol
                        << "_Exchange_" << exchange;

            return stream_obj.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return "";
    }

    string str()
    {
        try
        {
            std::stringstream stream_obj;

            stream_obj  << "[K-Kine] SRC " << get_sec_time_str(index)  
                        << ", " << exchange << ", "<< symbol << ","
                        << "open: " << px_open.get_value() << ", high: " << px_high.get_value() << ", "
                        << "low: " << px_low.get_value() << ", close: " << px_close.get_value() << "\n"; 

            return stream_obj.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return "";
    }


    string get_json_str()
    {
        try
        {
            nlohmann::json json_data;            

            json_data[0] = index;
            json_data[1] = px_open.get_value();
            json_data[2] = px_high.get_value();
            json_data[3] = px_low.get_value();
            json_data[4] = px_close.get_value();
            json_data[5] = volume.get_value();
            
            json_data[6] = symbol;
            json_data[7] = exchange;
            json_data[8] = resolution;

            return json_data.dump();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        return "";
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

    string meta_str() const
    {
        try
        {
            std::stringstream stream_obj;

            stream_obj  << "trade_" 
                        << "Symbol_" << symbol
                        << "_Exchange_" << exchange;

            return stream_obj.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return "";
    }

    string get_json_str()
    {
        try
        {
            nlohmann::json json_data;            

            json_data["Time"] = time;
            json_data["LastPx"] = price.get_value();
            json_data["Qty"] = volume.get_value();
            json_data["Exchange"] = exchange;
            json_data["Symbol"] = symbol;

            return json_data.dump();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        return "";
    }    

    string str()
    {
        try
        {
            std::stringstream stream_obj;

            stream_obj  << "Symbol: " << symbol
                        << ", Exchange: " << exchange                         
                        << ", Time " << get_sec_time_str(time)  
                        << ", LastPx" << price.get_value() 
                        << ", Qty: "<< volume.get_value() 
                        << "\n"; 

            return stream_obj.str();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return "";
    }
};