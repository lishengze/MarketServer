#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"

#include "comm_declare.h"
#include "comm_type_def.h"

COMM_NAMESPACE_START

class Comm
{
    public:
        Comm(string server_address, 
            QuoteSourceCallbackInterface* depth_engine,
            QuoteSourceCallbackInterface* kline_engine,
            QuoteSourceCallbackInterface* trade_engine,        
            NET_TYPE net_type = NET_TYPE::KAFKA,
            SERIALIZE_TYPE serialize_type = SERIALIZE_TYPE::JSON)
;

        Comm(string server_address,
            NET_TYPE net_type = NET_TYPE::KAFKA,
            SERIALIZE_TYPE serialize_type = SERIALIZE_TYPE::JSON,
            QuoteSourceCallbackInterface* engine_center=nullptr);   

        ~Comm();        

        void launch();

        void publish_depth(const SDepthQuote& quote);
        void publish_kline(const KlineData& kline);
        void publish_trade(const TradeData& trade);

        void set_meta(const MetaType kline_meta, const MetaType depth_meta, const MetaType trade_meta);

        void set_kline_meta(const MetaType meta);
        void set_depth_meta(const MetaType meta);
        void set_trade_meta(const MetaType meta);

    private:
        NetServer*          net_server_{nullptr};
        Serializer*         serializer_{nullptr};
};

DECLARE_PTR(Comm);

COMM_NAMESPACE_END