#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_type_def.h"

#include "interface_define.h"

COMM_NAMESPACE_START

class DepthProcessor:public QuoteSourceCallbackInterface
{
public:

    DepthProcessor(QuoteSourceCallbackInterface* engine);

    ~DepthProcessor();

    bool check(SDepthQuote& src);

    virtual void on_snap(SDepthQuote& src);

    bool store_first_quote(const SDepthQuote& src);

    bool is_sequenced_quote(const SDepthQuote& src);

    bool process_snap_quote(const SDepthQuote& src);

    bool process_update_quote(const SDepthQuote& src);

    void merge_update(SDepthQuote& snap, const SDepthQuote& update);

private:
    QuoteSourceCallbackInterface *              engine_{nullptr};

    map<TSymbol, map<TExchange, SDepthQuote>>   latest_depth_quote_;
};

DECLARE_PTR(DepthProcessor);

COMM_NAMESPACE_END