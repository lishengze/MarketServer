#include "decode_processer.h"
#include "Log/log.h"
#include "util/tool.h"

#include "depth_processor.h"
#include "trade_processor.h"
#include "kline_processor.h"

void EncodeProcesser::process_kline(const SDepthQuote& depth)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void EncodeProcesser::process_depth(const KlineData& kline)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void EncodeProcesser::process_trade(const TradeData& trade)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}


void DecodeProcesser::_json_to_quote_depth(const Value& data, const SExchangeConfig& config, map<SDecimal, SDepth>& depths)
{
    try
    {
        for (auto iter = data.MemberBegin() ; iter != data.MemberEnd() ; ++iter )
        {
            const string& price = iter->name.GetString();
            const double& volume = iter->value.GetDouble();
            SDecimal dPrice = SDecimal::parse(price, config.precise);
            SDecimal dVolume = SDecimal::parse(volume, config.vprecise);
            depths[dPrice].volume = dVolume;
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    

}

bool DecodeProcesser::_json_to_quote(const Document& snap_json, SDepthQuote& quote, const SExchangeConfig& config, bool isSnap) 
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
        
        quote.price_precise = config.precise;
        quote.volume_precise = config.vprecise;
        quote.amount_precise = config.aprecise;
        vassign(quote.sequence_no, sequence_no);
        vassign(quote.origin_time, parse_nano(timeArrive));
        quote.arrive_time = get_miliseconds();
        quote.server_time = 0; // 这个时间应该在发送前赋值
        quote.is_snap = isSnap;
        
        string askDepth = isSnap ? "AskDepth" : "AskUpdate";
        _json_to_quote_depth(snap_json[askDepth.c_str()], config, quote.asks);
        string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
        _json_to_quote_depth(snap_json[bidDepth.c_str()], config, quote.bids);
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
        return false;
    }
}

bool DecodeProcesser::_json_to_kline(const Value& data,  SExchangeConfig& config, KlineData& kline) 
{
    try
    {
        kline.symbol = data[7].GetString();
        kline.exchange = data[8].GetString();
        kline.index = int(data[0].GetDouble());
        kline.px_open.from(data[1].GetDouble(), config.precise);
        kline.px_high.from(data[2].GetDouble(), config.precise);
        kline.px_low.from(data[3].GetDouble(), config.precise);
        kline.px_close.from(data[4].GetDouble(), config.precise);
        kline.volume.from(data[5].GetDouble(), config.vprecise);

        // kline.resolution = data[9].GetInt64();
        return _is_kline_valid(kline);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
        return false;
    }
}

bool DecodeProcesser::_get_config(string symbol, string exchange, SExchangeConfig& config)
{
    try
    {
        if (symbol_config_.find(symbol) == symbol_config_.end())
        {
            return false;
        }
        if (symbol_config_[symbol].find(exchange) == symbol_config_[symbol].end())
        {
            return false;
        }

        config = symbol_config_[symbol][exchange];        
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
        return false;
    }
}

void DecodeProcesser::process_data(const std::vector<string>& src_data_vec)
{
    try
    {
        for (auto src_data:src_data_vec)
        {
            MetaData meta_data;
            if (!pre_process(src_data, meta_data)) continue;
            // LOG_INFO(meta_data.simple_str());

            SExchangeConfig config;
            if (!_get_config(meta_data.symbol, meta_data.exchange, config)) continue;
            
            if (meta_data.type == DEPTH_TYPE)
            {
                SDepthQuote depth_quote;
                if (decode_depth(meta_data.data_body, config, depth_quote))
                {
                    p_depth_processor_->process(depth_quote);
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
                if (decode_kline(meta_data.data_body, config, kline))
                {
                    p_kline_processor_->process(kline);
                }
                else
                {
                    LOG_WARN("decode kline faild, ori_msg: " + src_data);
                }
            }         
            else if (meta_data.type == TRADE_TYPE)
            {
                TradeData trade_data;
                if (decode_trade(meta_data.data_body, config, trade_data))
                {
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

bool DecodeProcesser::decode_depth(Document& json_data, SExchangeConfig& config, SDepthQuote& depth_quote)
{
    try
    {
        string symbol = json_data["Symbol"].GetString();
        string exchange = json_data["Exchange"].GetString();
        string type = json_data["Type"].GetString();
        bool is_snap = type=="snap" ? true:false;

        return _json_to_quote(json_data, depth_quote, config, is_snap);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

void DecodeProcesser::decode_kline(Document& json_data, SExchangeConfig& config, vector<KlineData>& klines)
{
    try
    {
        for (auto iter = json_data.Begin(); iter != json_data.End(); ++iter) 
        {
            KlineData kline;
            _json_to_kline(*iter, config, kline);
            if( !_is_kline_valid(kline) ) 
            {
                LOG_WARN("[kline min] get abnormal kline data " + std::to_string(kline.resolution));
                continue;
            }

            klines.push_back(kline);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

bool DecodeProcesser::decode_kline(Document& json_data, SExchangeConfig& config, KlineData& kline)
{
    try
    {
        return _json_to_kline(json_data, config, kline);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DecodeProcesser::decode_trade(Document& json_data, SExchangeConfig& config, TradeData& trade_data)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool DecodeProcesser::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    try
    {
        LOG_INFO("Set Config " + symbol + ":\n");
        for (auto iter:config)
        {
            LOG_INFO(iter.first + "\n" + iter.second.str());
        }

        symbol_config_[symbol] = config;
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

void DecodeProcesser::set_new_config(std::unordered_map<TSymbol, SSymbolConfig>& new_config)
{
    try
    {
        symbol_config_ = new_config;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}