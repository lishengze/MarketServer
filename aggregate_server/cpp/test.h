#pragma once
#include "kafka_server.h"
#include "decode_processer.h"
#include "pandora/util/io_service_pool.h"


class TestEngine
{
    public:
        TestEngine(utrade::pandora::io_service_pool& engine_pool):
        engine_pool_{engine_pool}
        {

        }

        void init();

        void start();

    private:
        KafkaQuote*                         p_kafka_{nullptr};
        DecodeProcesser*                    p_decode_processer_{nullptr}; 
        utrade::pandora::io_service_pool&   engine_pool_;
};


void TestMain();