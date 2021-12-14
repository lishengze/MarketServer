#include "decode_processer.h"
#include "Log/log.h"
#include "util/tool.h"
#include "kafka_server.h"

#include "depth_processor.h"
#include "trade_processor.h"
#include "kline_processor.h"

void EncodeProcesser::on_snap( SDepthQuote& depth)
{
    try
    {
        string json_str = depth.get_json_str();
        string topic = get_depth_topic(depth.exchange, depth.symbol);

        if (kafka_server_)
        {
            kafka_server_->publish_msg(topic, json_str);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void EncodeProcesser::on_kline( KlineData& kline)
{
    try
    {
        string json_str = kline.get_json_str();
        string topic = get_kline_topic(kline.exchange, kline.symbol);

        // LOG_INFO(topic + ": " + json_str);

        if (kafka_server_)
        {
            kafka_server_->publish_msg(topic, json_str);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void EncodeProcesser::on_trade( TradeData& trade)
{
    try
    {
        string json_str = trade.get_json_str();
        string topic = get_trade_topic(trade.exchange, trade.symbol);

        if (kafka_server_)
        {
            kafka_server_->publish_msg(topic, json_str);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DecodeProcesser::_json_to_quote_depth(const Value& data, map<SDecimal, SDepth>& depths)
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
       LOG_ERROR(e.what());
    }    
}

bool DecodeProcesser::_json_to_quote(const Document& snap_json, SDepthQuote& quote, bool isSnap)
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
        LOG_ERROR(e.what());
    }
    return false;
}

bool DecodeProcesser::_json_to_kline(const Value& data, KlineData& kline)
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
        LOG_ERROR(e.what());
    }

    return false;
}

bool DecodeProcesser::is_data_subed(const string symbol, const string exchange)
{
    try
    {
        if(sub_info_map_.find(symbol) != sub_info_map_.end() &&
            sub_info_map_[symbol].find(exchange) != sub_info_map_[symbol].end())
            return true;

        return false;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return false;
}

void DecodeProcesser::process_data(const std::vector<string>& src_data_vec)
{
    try
    {
        for (auto src_data:src_data_vec)
        {
            MetaData meta_data;
            if (!pre_process(src_data, meta_data)) continue;
            LOG_INFO(meta_data.simple_str());

            if (!is_data_subed(meta_data.symbol, meta_data.exchange)) 
            {
                LOG_WARN("data is not subed, symbol: " + meta_data.symbol + ", exchange: " + meta_data.exchange);
                continue;
            }
            
            if (meta_data.type == DEPTH_TYPE)
            {
                SDepthQuote depth_quote;
                if (decode_depth(meta_data.data_body, depth_quote))
                {
                    // p_depth_processor_->process(depth_quote);
                    // LOG_INFO(quote_str(depth_quote));
                }
                else
                {
                    LOG_WARN("decode depth faild, ori_msg: " + src_data);
                }                
            }
            else if (meta_data.type == KLINE_TYPE)
            {
                KlineData kline;
                if (decode_kline(meta_data.data_body, kline))
                {
                    p_kline_processor_->process(kline);
                    // LOG_INFO(kline.get_json_str());
                }
                else
                {
                    LOG_WARN("decode kline faild, ori_msg: " + src_data);
                }
            }         
            else if (meta_data.type == TRADE_TYPE)
            {
                TradeData trade_data;
                if (decode_trade(meta_data.data_body, trade_data))
                {
                    LOG_INFO(trade_data.get_json_str());
                    p_trade_processor_->process(trade_data);
                }
                else
                {
                    LOG_WARN("decode trade faild, ori_msg: " + src_data);
                }
            }                  
            else 
            {
                LOG_WARN("Unknown Topic: " + (meta_data.type));
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool DecodeProcesser::pre_process(const string& src_data, MetaData& meta_data)
{
    try
    {
        string::size_type topic_end_pos = src_data.find(TOPIC_SEPARATOR);
        if (topic_end_pos == std::string::npos)
        {
            LOG_WARN("Cann't Locate TOPIC_SEPARATOR " + TOPIC_SEPARATOR + ", SrCData: " + src_data);
            return false;
        }
        string topic = src_data.substr(0, topic_end_pos);
        
        std::string::size_type type_end_pos = topic.find(TYPE_SEPARATOR);
        if( type_end_pos == std::string::npos )
        {
            LOG_WARN("Cann't Locate TYPE_SEPARATOR " + TYPE_SEPARATOR + ", topic: " + topic);
            return false;            
        }
        meta_data.type = topic.substr(0, type_end_pos);

        string symbol_exchange = topic.substr(type_end_pos+1);

        std::string::size_type symbol_end_pos = symbol_exchange.find(SYMBOL_EXCHANGE_SEPARATOR);
        if( symbol_end_pos == std::string::npos)
        {
            LOG_WARN("Cann't Locate SYMBOL_EXCHANGE_SEPARATOR " + SYMBOL_EXCHANGE_SEPARATOR + ", symbol_exchange: " + symbol_exchange);
            return false;
        }
            
        meta_data.symbol = symbol_exchange.substr(0, symbol_end_pos);
        meta_data.exchange = symbol_exchange.substr(symbol_end_pos + 1);
                
        string data_body_str = src_data.substr(topic_end_pos+1);                
        meta_data.data_body.Parse(data_body_str.c_str());
        if(meta_data.data_body.HasParseError())
        {
            LOG_WARN("Document Parse " + data_body_str+ " Failed");
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DecodeProcesser::decode_depth(Document& json_data, SDepthQuote& depth_quote)
{
    try
    {
        string symbol = json_data["Symbol"].GetString();
        string exchange = json_data["Exchange"].GetString();
        string type = json_data["Type"].GetString();
        bool is_snap = type=="snap" ? true:false;

        return _json_to_quote(json_data, depth_quote, is_snap);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DecodeProcesser::decode_kline(Document& json_data, KlineData& kline)
{
    try
    {
        return _json_to_kline(json_data, kline);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DecodeProcesser::decode_trade(Document& json_data,TradeData& trade_data)
{
    try
    {
        trade_data.time = parse_nano(json_data["Time"].GetString());  // 2020-12-27 12:48:41.578000
        trade_data.price.from(json_data["LastPx"].GetDouble());
        trade_data.volume.from(json_data["Qty"].GetDouble());

        trade_data.exchange = json_data["Exchange"].GetString();
        trade_data.symbol = json_data["Symbol"].GetString();

        return is_trade_valid(trade_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

void DecodeProcesser::set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& sub_info_map)
{
    try
    {
        sub_info_map_ = sub_info_map;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}