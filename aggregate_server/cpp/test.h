#pragma once
#include "kafka_server.h"
#include "decode_processer.h"
#include "depth_processor.h"
#include "kline_processor.h"

#include "depth_aggregater.h"

#include "pandora/util/io_service_pool.h"
#include "configuration_client.h"
#include "interface_define.h"

class TestEngine:public INacosCallback
{
    public:
        TestEngine(utrade::pandora::io_service_pool& engine_pool):
        engine_pool_{engine_pool}
        {
            init();
        }

        ~TestEngine();

        void init();

        void start();

        virtual void on_config_channged(const Document& symbols);

    private:
        KafkaServer*                                    p_kafka_{nullptr};
        DecodeProcesser*                                p_decode_processer_{nullptr}; 

        DepthProcessor*                                 p_depth_processor_{nullptr};
        KlineProcessor*                                 p_kline_processor_{nullptr};

        DepthAggregater*                                p_depth_aggregater_{nullptr};

        utrade::pandora::io_service_pool&               engine_pool_;
        ConfigurationClient                             config_client_;

        std::unordered_map<TSymbol, SNacosConfig>       nacos_config_;
        std::unordered_map<TSymbol, SSymbolConfig>      trans_config_;
};


void TestMain();