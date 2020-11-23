#include "kline_server.h"
#include "kline_server_config.h"


// config file relative path
const char* config_file = "config.json";

KlineServer::KlineServer()
{
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
}

KlineServer::~KlineServer()
{
}

void KlineServer::start() 
{
    kline_db_.start();
    server_endpoint_.start();
    
    kline_mixer_.register_callback(&kline_db_);
    kline_mixer_.register_callback(&server_endpoint_);
    kline_mixer_.set_db_interface(&kline_db_);
    kline_mixer_.start();

    kline_source_.register_callbakc(&kline_mixer_);
    kline_source_.start();
}

void KlineServer::signal_handler(int signum)
{
    //UT_LOG_INFO(GALAXY_LOGGER, "KernelEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    //KERNELENGINE->release();
    // 退出
    exit(0);
}
