#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "pandora/util/io_service_pool.h"
#include "comm_type_def.h"
#include "comm_interface_define.h"
#include "comm_log.h"

#include "depth_processor.h"
#include "kline_processor.h"
#include "trade_processor.h"

COMM_NAMESPACE_START

class JsonSerializer:public Serializer
{
public:

    JsonSerializer(QuoteSourceCallbackInterface* depth_engine, 
                    QuoteSourceCallbackInterface* kline_engine, 
                    QuoteSourceCallbackInterface* trade_engine)
    {
        p_depth_processor_ = boost::make_shared<DepthProcessor>(depth_engine);
        p_kline_processor_ = boost::make_shared<KlineProcessor>(kline_engine);
        p_trade_processor_ = boost::make_shared<TradeProcessor>(trade_engine);
    }    

    JsonSerializer(QuoteSourceCallbackInterface* engine_center)
    {
        p_depth_processor_ = boost::make_shared<DepthProcessor>(engine_center);
        p_kline_processor_ = boost::make_shared<KlineProcessor>(engine_center);
        p_trade_processor_ = boost::make_shared<TradeProcessor>(engine_center);        
    }    


    virtual ~JsonSerializer() { }

    // UnSerialize
    virtual void on_snap(const string& src);
    virtual void on_kline(const string& src);
    virtual void on_trade(const string& src);

    // Serialize
    virtual string on_snap(const SDepthQuote& quote);
    virtual string on_kline(const KlineData& kline);
    virtual string on_trade(const TradeData& trade);

    bool decode_trade(const string& src, TradeData& trade_data);
    bool decode_depth(const string& src, SDepthQuote& depth_quote);
    bool decode_kline(const string& src, KlineData& klines);

public:
    void _json_to_quote_depth(const Value& data, map<SDecimal, SDepth>& depths);

    bool _json_to_quote(const Document& snap_json, SDepthQuote& quote, bool isSnap);

    bool _json_to_kline(const Value& data, KlineData& kline);

private:
    DepthProcessorPtr                         p_depth_processor_{nullptr};
    
    KlineProcessorPtr                         p_kline_processor_{nullptr};

    TradeProcessorPtr                         p_trade_processor_{nullptr};

    mutable std::mutex                                    mutex_symbol_;

    std::unordered_map<TSymbol, std::set<TExchange>>      sub_info_map_;
};

DECLARE_PTR(JsonSerializer);

COMM_NAMESPACE_END