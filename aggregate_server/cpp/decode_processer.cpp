#include "decode_processer.h"
#include "../Log/log.h"

void redisquote_to_quote_depth(const Value& data, const SExchangeConfig& config, map<SDecimal, SDepth>& depths)
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

bool redisquote_to_quote(const Document& snap_json, SDepthQuote& quote, const SExchangeConfig& config, bool isSnap) {
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
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth.c_str()], config, quote.asks);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth.c_str()], config, quote.bids);
    return true;
}

void DecodeProcesser::process_data(const std::vector<string>& src_data_vec)
{
    try
    {
        for (auto src_data:src_data_vec)
        {
            string topic;
            string data_body;

            pre_process(src_data, topic, data_body);

            Document json_body;
            json_body.Parse(data_body.c_str());
            if(json_body.HasParseError())
            {
                LOG_WARN("Document Parse " + data_body + " Failed");
                continue;
            }

            if (topic == "depth")
            {
                SDepthQuote depth_quote;

            }
            else if (topic == "kline")
            {
                
            }            
            else 
            {
                LOG_WARN("Unknown Topic: " + topic);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DecodeProcesser::pre_process(const string& src_data, string& topic, string& data_body)
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

void DecodeProcesser::decode_depth(Document& json_data, SDepthQuote& depth_quote)
{
    try
    {
        string symbol = json_data["Symbol"].GetString();
        string exchange = json_data["Exchange"].GetString();
        string type = json_data["Type"].GetString();
        bool is_snap = type=="snap" ? true:false;

        if (symbol_config_.find(symbol) == symbol_config_.end())
        {
            return ;
        }
        if (symbol_config_[symbol].find(exchange) != symbol_config_[symbol].end())
        {
            return ;
        }        

        SExchangeConfig& config = symbol_config_[symbol][exchange];
        redisquote_to_quote(json_data, depth_quote, config, is_snap);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
}

bool redisquote_to_kline(const Value& data, KlineData& kline) 
{
    
    kline.symbol = data[7].GetString();
    kline.exchange = data[8].GetString();

    if (symbol_config_.find(kline.symbol) == symbol_config_.end())
    {
        return false;
    }
    if (symbol_config_[kline.symbol].find(kline.exchange) != symbol_config_[kline.symbol].end())
    {
        return false;
    }

    SExchangeConfig& config = symbol_config_[kline.symbol][kline.exchange];

    kline.index = int(data[0].GetDouble());
    kline.px_open.from(data[1].GetDouble(), config.precise);
    kline.px_high.from(data[2].GetDouble(), config.precise);
    kline.px_low.from(data[3].GetDouble(), config.precise);
    kline.px_close.from(data[4].GetDouble(), config.precise);
    kline.volume.from(data[5].GetDouble(), config.vprecise);


    kline.resolution = data[9].GetInt64();
    return true;
}

bool valida_kline(const KlineData& kline) {
    return !(kline.index < 1000000000 || kline.index > 1900000000);
}

void DecodeProcesser::decode_kline(Document& json_data, vector<KlineData>& klines)
{
    try
    {
        for (auto iter = json_data.Begin(); iter != json_data.End(); ++iter) 
        {
            KlineData kline;
            redisquote_to_kline(*iter, kline);
            if( !valida_kline(kline) ) 
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
        std::cerr << e.what() << '\n';
    }

}