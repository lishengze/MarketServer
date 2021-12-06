#pragma once

#include "global_declare.h"

#include "struct_define.h"

#include "depth_aggregater.h"

class DepthProcessor
{
public:

    DepthProcessor(DepthAggregater* );

    ~DepthProcessor();

    void process(const SDepthQuote& src);

    bool store_first_quote(const SDepthQuote& src);

    bool is_sequenced_quote(const SDepthQuote& src);

    bool process_snap_quote(const SDepthQuote& src);

    bool process_update_quote(const SDepthQuote& src);

    void merge_update(SDepthQuote& snap, const SDepthQuote& update);
private:

    DepthAggregater*                            p_aggregater_{nullptr};
    map<TSymbol, map<TExchange, SDepthQuote>>   latest_depth_quote_;
};