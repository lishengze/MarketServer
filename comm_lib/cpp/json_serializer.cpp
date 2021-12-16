#include "base/cpp/base_data_stuct.h"
#include "json_serializer.h"
#include "base/cpp/util.h"

#include "depth_processor.h"
#include "trade_processor.h"
#include "kline_processor.h"

#include "util.h"
#include "base/cpp/quote.h"

COMM_NAMESPACE_START

string JsonSerializer::on_snap(const SDepthQuote& depth)
{
    try
    {
        string json_str = get_depth_jsonstr(depth);

        // if (depth.symbol == "BTC_USDT")
        // {
        //     COMM_LOG_INFO(depth.str());
        // }
        
        return json_str;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }

    return "";
}

string JsonSerializer::on_kline(const KlineData& kline)
{
    try
    {
        string json_str = get_kline_jsonstr(kline);

        // COMM_LOG_INFO(topic + ": " + json_str);

        return json_str;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
    return "";
}

string JsonSerializer::on_trade(const TradeData& trade)
{
    try
    {
        string json_str = get_trade_jsonstr(trade);

        return json_str;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
    return "";
}

void JsonSerializer::_json_to_quote_depth(const Value& data, map<SDecimal, SDepth>& depths)
{
    try
    {
        for (auto iter = data.MemberBegin() ; iter != data.MemberEnd() ; ++iter )
        {
            const string& price = iter->name.GetString();
            const double& volume = iter->value.GetDouble();
            SDecimal dPrice = SDecimal::parse(price);
            SDecimal dVolume = SDecimal::parse(volume);
            depths[dPrice].volume = dVolume;
        }
    }
    catch(const std::exception& e)
    {
       COMM_LOG_ERROR(e.what());
    }    
}

bool JsonSerializer::_json_to_quote(const Document& snap_json, SDepthQuote& quote, bool isSnap)
{
    try
    {
        quote.asks.clear();
        quote.bids.clear();
        
        // 使用channel里面的交易所和代码，不使用json中的
        //string symbol = snap_json["Symbol"].get<std::string>();
        //string exchange = snap_json["Exchange"].get<std::string>();
        string timeArrive = snap_json["TimeArrive"].GetString();
        type_seqno sequence_no = snap_json["Msg_seq_symbol"].GetUint64(); 
    
        vassign(quote.sequence_no, sequence_no);
        vassign(quote.origin_time, parse_nano(timeArrive));
        quote.arrive_time = get_miliseconds();
        quote.server_time = 0; // 这个时间应该在发送前赋值
        quote.is_snap = isSnap;
        
        string askDepth = isSnap ? "AskDepth" : "AskUpdate";
        _json_to_quote_depth(snap_json[askDepth.c_str()], quote.asks);
        string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
        _json_to_quote_depth(snap_json[bidDepth.c_str()], quote.bids);
        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool JsonSerializer::_json_to_kline(const Value& data, KlineData& kline)
{
    try
    {

        kline.index = int(data[0].GetDouble());
        kline.px_open.from(data[1].GetDouble());
        kline.px_high.from(data[2].GetDouble());
        kline.px_low.from(data[3].GetDouble());
        kline.px_close.from(data[4].GetDouble());
        kline.volume.from(data[5].GetDouble());

        kline.symbol = data[7].GetString();
        kline.exchange = data[8].GetString();
        kline.resolution = data[9].GetInt64();
        return is_kline_valid(kline);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }

    return false;
}

// 行情接口
void JsonSerializer::on_snap(const string& src) 
{
    try
    {
        SDepthQuote depth_quote;
        if (decode_depth(src, depth_quote))
        {
            // if (depth_quote.symbol == "BTC_USDT")
            // {
            //     COMM_LOG_INFO(quote_str(depth_quote, 5));
            // }                    
            p_depth_processor_->on_snap(depth_quote);
            
        }
        else
        {
            COMM_LOG_WARN("decode depth faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
};

// K线接口
void JsonSerializer::on_kline(const string& src)
{
    try
    {
        KlineData kline;
        if (decode_kline(src, kline))
        {
            p_kline_processor_->on_kline(kline);
            // COMM_LOG_INFO(kline.get_json_str());
        }
        else
        {
            COMM_LOG_WARN("decode kline faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

// 交易接口
void JsonSerializer::on_trade(const string& src)
{
    try
    {
        TradeData trade_data;
        if (decode_trade(src, trade_data))
        {
            // COMM_LOG_INFO(trade_data.get_json_str());
            p_trade_processor_->on_trade(trade_data);
        }
        else
        {
            COMM_LOG_WARN("decode trade faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

bool JsonSerializer::decode_depth(const string& src, SDepthQuote& depth_quote)
{
    try
    {
        Document json_data;
        json_data.Parse(src.c_str());
        if (json_data.HasParseError()) 
        {
            COMM_LOG_WARN("Document Parse " + src + " Failed");
            return false;            
        }

        string symbol = json_data["Symbol"].GetString();
        string exchange = json_data["Exchange"].GetString();
        string type = json_data["Type"].GetString();
        bool is_snap = type=="snap" ? true:false;

        depth_quote.symbol = symbol;
        depth_quote.exchange = exchange;
        depth_quote.is_snap = is_snap;

        return _json_to_quote(json_data, depth_quote, is_snap);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool JsonSerializer::decode_kline(const string& src, KlineData& kline)
{
    try
    {
        Document json_data;
        json_data.Parse(src.c_str());
        if (json_data.HasParseError()) 
        {
            COMM_LOG_WARN("Document Parse " + src + " Failed");
            return false;            
        }

        return _json_to_kline(json_data, kline);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool JsonSerializer::decode_trade(const string& src,TradeData& trade_data)
{
    try
    {
        Document json_data;
        json_data.Parse(src.c_str());
        if (json_data.HasParseError()) 
        {
            COMM_LOG_WARN("Document Parse " + src + " Failed");
            return false;            
        }

        trade_data.time = parse_nano(json_data["Time"].GetString());  // 2020-12-27 12:48:41.578000
        trade_data.price.from(json_data["LastPx"].GetDouble());
        trade_data.volume.from(json_data["Qty"].GetDouble());

        trade_data.exchange = json_data["Exchange"].GetString();
        trade_data.symbol = json_data["Symbol"].GetString();

        return is_trade_valid(trade_data);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

COMM_NAMESPACE_END