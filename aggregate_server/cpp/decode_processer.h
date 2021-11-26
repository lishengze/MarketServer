#pragma once
#include "global_declare.h"
#include "pandora/util/io_service_pool.h"
#include "pandora/util/json.hpp"
#include "struct_define.h"

class DecodeProcesser
{
public:
    DecodeProcesser(utrade::pandora::io_service_pool& process_pool):
        process_pool_{process_pool}
    {}


    void process_data(const std::vector<string>& src_data);

    void pre_process(const string& src_data, string& topic, string& data_body);

    void decode_depth(Document& json_data, SDepthQuote& depth_quote);

    void decode_kline(Document& json_data, vector<KlineData>& klines);

    bool set_config(const TSymbol& symbol, const SSymbolConfig& config);

private:
    utrade::pandora::io_service_pool&       process_pool_;
    
    mutable std::mutex                      mutex_symbol_;
    unordered_map<TSymbol, SSymbolConfig>   symbol_config_;
};