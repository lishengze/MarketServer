#pragma once
#include "global_declare.h"
#include "pandora/util/io_service_pool.h"
#include "struct_define.h"
#include "interface_define.h"



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
    DepthProcessor*                         p_depth_processor_{nullptr};
    
    KlineProcessor*                         p_kline_processor_{nullptr};

    TradeProcessor*                         p_trade_processor_{nullptr};

    utrade::pandora::io_service_pool&       process_pool_;

    mutable std::mutex                      mutex_symbol_;

    std::unordered_map<TSymbol, std::set<TExchange>> sub_info_map_;
};