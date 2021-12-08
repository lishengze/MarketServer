#pragma once
#include "global_declare.h"
#include "pandora/util/io_service_pool.h"
#include "struct_define.h"



class DepthProcessor;
class KlineProcessor;
class TradeProcessor;


class EncodeProcesser
{
public:

    void process_kline(const SDepthQuote& depth);

    void process_depth(const KlineData& kline);

    void process_trade(const TradeData& trade);

private:

};

class DecodeProcesser
{
public:
    DecodeProcesser(utrade::pandora::io_service_pool& process_pool):
        process_pool_{process_pool}
    {}

    DecodeProcesser(DepthProcessor* depth_processor, 
                    KlineProcessor* kline_processor, 
                    TradeProcessor* trade_processor,
                    utrade::pandora::io_service_pool& process_pool):
        p_depth_processor_{depth_processor}, 
        p_kline_processor_{kline_processor},
        p_trade_processor_{trade_processor},
        process_pool_{process_pool}
    {}    

    virtual ~DecodeProcesser() { }

    struct MetaData
    {
        string      type;
        Document    data_body;
        string      symbol;
        string      exchange;

        string simple_str()
        {
            return string("type: ") + type + ", symbol: " + symbol + ", exchange: " + exchange;
        }
    };

    void process_data(const std::vector<string>& src_data);

    bool pre_process(const string& src_data, MetaData& meta_data);

    bool decode_trade(Document& json_data, SExchangeConfig& config, TradeData& trade_data);

    bool decode_depth(Document& json_data, SExchangeConfig& config, SDepthQuote& depth_quote);

    void decode_kline(Document& json_data, SExchangeConfig& config, vector<KlineData>& klines);

    bool decode_kline(Document& json_data, SExchangeConfig& config, KlineData& klines);

    bool set_config(const TSymbol& symbol, const SSymbolConfig& config);

    void set_new_config(std::unordered_map<TSymbol, SSymbolConfig>& new_config);

public:
    void _json_to_quote_depth(const Value& data, const SExchangeConfig& config, map<SDecimal, SDepth>& depths);

    bool _json_to_quote(const Document& snap_json, SDepthQuote& quote, const SExchangeConfig& config, bool isSnap);

    bool _json_to_kline(const Value& data,  SExchangeConfig& config, KlineData& kline);

    bool _get_config(string symbole, string exchange, SExchangeConfig& config);


private:
    DepthProcessor*                         p_depth_processor_{nullptr};
    
    KlineProcessor*                         p_kline_processor_{nullptr};

    TradeProcessor*                         p_trade_processor_{nullptr};

    utrade::pandora::io_service_pool&       process_pool_;

    mutable std::mutex                      mutex_symbol_;
    unordered_map<TSymbol, SSymbolConfig>   symbol_config_;
};