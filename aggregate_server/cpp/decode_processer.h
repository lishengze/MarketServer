#pragma once
#include "global_declare.h"
#include "pandora/util/io_service_pool.h"
#include "struct_define.h"

class DecodeProcesser
{
public:
    DecodeProcesser(utrade::pandora::io_service_pool& process_pool):
        process_pool_{process_pool}
    {}

    virtual ~DecodeProcesser() { }

    struct MetaData
    {
        string      type;
        Document    data_body;
        string      symbol;
        string      exchange;
    };

    void process_data(const std::vector<string>& src_data);

    bool pre_process(const string& src_data, MetaData& meta_data);

    void decode_depth(Document& json_data, SExchangeConfig& config, SDepthQuote& depth_quote);

    void decode_kline(Document& json_data, SExchangeConfig& config, vector<KlineData>& klines);

    bool set_config(const TSymbol& symbol, const SSymbolConfig& config);


public:
    void _json_to_quote_depth(const Value& data, const SExchangeConfig& config, map<SDecimal, SDepth>& depths);

    bool _json_to_quote(const Document& snap_json, SDepthQuote& quote, const SExchangeConfig& config, bool isSnap);

    bool _is_kline_valid(const KlineData& kline);

    bool _json_to_kline(const Value& data,  SExchangeConfig& config, KlineData& kline);

    bool _get_config(string symbole, string exchange, SExchangeConfig& config);


private:
    utrade::pandora::io_service_pool&       process_pool_;

    mutable std::mutex                      mutex_symbol_;
    unordered_map<TSymbol, SSymbolConfig>   symbol_config_;
};