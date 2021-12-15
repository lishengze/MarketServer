#pragma once
#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "pandora/util/io_service_pool.h"

#include "interface_define.h"
#include "comm_log.h"

class DepthProcessor;
class KlineProcessor;
class TradeProcessor;
class KafkaServer;


class EncodeProcesser:public QuoteSourceCallbackInterface
{
public:

    virtual void on_snap( SDepthQuote& quote);

    // K线接口
    virtual void on_kline( KlineData& kline);

    // 交易接口
    virtual void on_trade( TradeData& trade);

    void set_kafka_server(KafkaServer* kafka_server) {
        kafka_server_ = kafka_server;
    }

private:
    KafkaServer*    kafka_server_{nullptr};
};

class DecodeProcesser
{
public:

    DecodeProcesser(QuoteSourceCallbackInterface* depth_processor, 
                    QuoteSourceCallbackInterface* kline_processor, 
                    QuoteSourceCallbackInterface* trade_processor):
        p_depth_processor_{depth_processor}, 
        p_kline_processor_{kline_processor},
        p_trade_processor_{trade_processor}
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

    bool is_data_subed(const string symbol, const string exchange);

    void process_data(const std::vector<string>& src_data);

    bool pre_process(const string& src_data, MetaData& meta_data);

    bool decode_trade(Document& json_data, TradeData& trade_data);

    bool decode_depth(Document& json_data,  SDepthQuote& depth_quote);

    bool decode_kline(Document& json_data, KlineData& klines);

    void set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& sub_info_map);

public:
    void _json_to_quote_depth(const Value& data, map<SDecimal, SDepth>& depths);

    bool _json_to_quote(const Document& snap_json, SDepthQuote& quote, bool isSnap);

    bool _json_to_kline(const Value& data, KlineData& kline);

private:
    QuoteSourceCallbackInterface*                         p_depth_processor_{nullptr};
    
    QuoteSourceCallbackInterface*                         p_kline_processor_{nullptr};

    QuoteSourceCallbackInterface*                         p_trade_processor_{nullptr};

    mutable std::mutex                                    mutex_symbol_;

    std::unordered_map<TSymbol, std::set<TExchange>>      sub_info_map_;
};

DECLARE_PTR(DecodeProcesser);