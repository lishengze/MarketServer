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

#include "market_data.pb.h"

using PDecimal = Proto3::MarketData::Decimal;
using PDepth = Proto3::MarketData::Depth;
using PDepthQuote = Proto3::MarketData::DepthQuote;
using PKlineData = Proto3::MarketData::KlineData;
using PTradeData = Proto3::MarketData::TradeData;
using PRepeatedDepth = google::protobuf::RepeatedPtrField<PDepth>;
using PDepthMap = google::protobuf::Map<std::string, Proto3::MarketData::Decimal>;

COMM_NAMESPACE_START

class ProtobufSerializer:public Serializer
{
public:

    ProtobufSerializer(QuoteSourceCallbackInterface* depth_engine, 
                    QuoteSourceCallbackInterface* kline_engine, 
                    QuoteSourceCallbackInterface* trade_engine)
    {
        p_depth_processor_ = boost::make_shared<DepthProcessor>(depth_engine);
        p_kline_processor_ = boost::make_shared<KlineProcessor>(kline_engine);
        p_trade_processor_ = boost::make_shared<TradeProcessor>(trade_engine);
    }    

    ProtobufSerializer(QuoteSourceCallbackInterface* engine_center)
    {
        p_depth_processor_ = boost::make_shared<DepthProcessor>(engine_center);
        p_kline_processor_ = boost::make_shared<KlineProcessor>(engine_center);
        p_trade_processor_ = boost::make_shared<TradeProcessor>(engine_center);        
    }    


    virtual ~ProtobufSerializer() { }

    // UnSerialize
    virtual void on_snap(const string& src);
    virtual void on_kline(const string& src);
    virtual void on_trade(const string& src);

    // Serialize
    virtual string on_snap(const SDepthQuote& quote);
    virtual string on_kline(const KlineData& kline);
    virtual string on_trade(const TradeData& trade);

    bool decode_depth(PDepthQuote& src, SDepthQuote& depth_quote);
    bool decode_kline(PKlineData& src, KlineData& klines);    
    bool decode_trade(PTradeData& src, TradeData& trade_data);

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

DECLARE_PTR(ProtobufSerializer);

COMM_NAMESPACE_END