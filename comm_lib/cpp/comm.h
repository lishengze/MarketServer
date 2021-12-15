#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "interface_define.h"

#include "comm_declare.h"
#include "comm_type_def.h"

#include "kafka_server.h"

COMM_NAMESPACE_START

class Comm
{
    public:
        Comm(string server_address, 
            QuoteSourceCallbackInterface* depth_engine = nullptr,
            QuoteSourceCallbackInterface* kline_engine = nullptr,
            QuoteSourceCallbackInterface* trade_engine = nullptr);

        void launch();

        void set_meta(const MetaType kline_meta, const MetaType depth_meta, const MetaType trade_meta);

        void set_kline_meta(const MetaType meta);
        void set_depth_meta(const MetaType meta);
        void set_trade_meta(const MetaType meta);

    private:
        KafkaServerPtr          kafka_sptr_{nullptr};
};

COMM_NAMESPACE_END