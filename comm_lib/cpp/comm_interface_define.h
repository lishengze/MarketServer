#pragma once 

#include "base/cpp/base_data_stuct.h"

#include "comm_log.h"
#include "comm_type_def.h"


COMM_NAMESPACE_START
class GrpcServerInterface
{
    public:

    void start() {}

    bool get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data) {}
};

class QuoteSourceCallbackInterface
{
public:

    virtual ~QuoteSourceCallbackInterface() {}

    // 行情接口
    virtual void on_snap( SDepthQuote& quote) { };

    // K线接口
    virtual void on_kline( KlineData& kline) { };

    // 交易接口
    virtual void on_trade( TradeData& trade) { };

    // 交易所异常无数据
    virtual void on_nodata_exchange(const TExchange& exchange){};
};

class Serializer
{
    public:

    virtual ~Serializer() { }
    virtual string on_snap(const SDepthQuote& quote) { return "";};

    // K线接口
    virtual string on_kline(const KlineData& kline) {return "";};

    // 交易接口
    virtual string on_trade(const TradeData& trade) {return "";};

    // 行情接口
    virtual void on_snap(const string& src) { };

    // K线接口
    virtual void on_kline(const string& src) { };

    // 交易接口
    virtual void on_trade(const string& src) { };    
};

class NetServer
{
    public:
        NetServer(Serializer* extern_serializer): serializer_{extern_serializer}
        {}

        virtual ~NetServer() {}

        virtual void launch(){ }

        virtual void set_meta(const MetaType meta) { }
        virtual void set_meta(const MetaType kline_meta, 
                              const MetaType depth_meta, 
                              const MetaType trade_meta)
        {}

        virtual void set_kline_meta(const MetaType meta) {}
        virtual void set_depth_meta(const MetaType meta) {}
        virtual void set_trade_meta(const MetaType meta) {}

        virtual void publish_depth(const SDepthQuote& quote) {}
        virtual void publish_kline(const KlineData& kline) {}
        virtual void publish_trade(const TradeData& trade) {}      

    public:
        Serializer*             serializer_{nullptr};
};



COMM_NAMESPACE_END