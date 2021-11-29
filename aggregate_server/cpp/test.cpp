#include "test.h"
#include "global_declare.h"
#include "Log/log.h"
#include "pandora/util/io_service_pool.h"
#include "stream_engine_config.h"

void TestEngine::init()
{
    try
    {
        p_decode_processer_ = new DecodeProcesser(engine_pool_);

        p_kafka_ = new KafkaServer(p_decode_processer_);
        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }  
}

void TestEngine::start()
{
    try
    {
        p_kafka_->launch();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}



void test_engine()
{
    string config_file_name = "config.json";
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file_name);

    utrade::pandora::io_service_pool engine_pool{3};

    TestEngine test_obj(engine_pool);
    test_obj.start();

    engine_pool.start();
    engine_pool.block();
}

void TestMain()
{
    test_engine();
}