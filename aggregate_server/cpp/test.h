#pragma once
#include "kafka_server.h"
#include "decode_processer.h"
#include "pandora/util/io_service_pool.h"
#include "configuration_client.h"
#include "stream_engine_define.h"

class TestEngine:public INacosCallback
{
    public:
        TestEngine(utrade::pandora::io_service_pool& engine_pool):
        engine_pool_{engine_pool}
        {
            init();
        }

        void init();

        void start();

        virtual void on_config_channged(const Document& symbols);

    private:
        KafkaServer*                                p_kafka_{nullptr};
        DecodeProcesser*                            p_decode_processer_{nullptr}; 
        utrade::pandora::io_service_pool&           engine_pool_;
        ConfigurationClient                         config_client_;

        std::unordered_map<TSymbol, SNacosConfig>    nacos_config_;
        std::unordered_map<TSymbol, SSymbolConfig>   trans_config_;
};


void TestMain();