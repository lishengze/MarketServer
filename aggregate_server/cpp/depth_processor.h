#pragma once

#include "global_declare.h"

#include "struct_define.h"

class DepthProcessor
{
public:

    DepthProcessor();

    ~DepthProcessor();

    void process(const SDepthQuote& src);

    bool store_first_quote(const SDepthQuote& src);

    bool is_sequenced_quote(const SDepthQuote& src);

    bool process_snap_quote(const SDepthQuote& src);

    bool process_update_quote(const SDepthQuote& src);

    void merge_update(SDepthQuote& snap, const SDepthQuote& update);

    void aggregate_quote(const SDepthQuote& snap);

private:
    map<string, map<string, SDepthQuote>>   latest_depth_quote_;
};