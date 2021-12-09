#pragma once

#include "global_declare.h"

#include "struct_define.h"

#include "depth_aggregater.h"

#include "interface_define.h"

class DepthProcessor
{
public:

    DepthProcessor(QuoteSourceCallbackInterface* engine);

    ~DepthProcessor();

    void process(SDepthQuote& src);

    bool store_first_quote(const SDepthQuote& src);

    bool is_sequenced_quote(const SDepthQuote& src);

    bool process_snap_quote(const SDepthQuote& src);

    bool process_update_quote(const SDepthQuote& src);

    void merge_update(SDepthQuote& snap, const SDepthQuote& update);

    void set_config(unordered_map<TSymbol, SMixerConfig> & new_config);

private:
    QuoteSourceCallbackInterface *              engine_{nullptr};

    DepthAggregater*                            p_aggregater_{nullptr};

    map<TSymbol, map<TExchange, SDepthQuote>>   latest_depth_quote_;

    unordered_map<TSymbol, SMixerConfig>        symbol_config_;
};