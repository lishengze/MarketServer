#include "server_engine.h"
#include "front_server_declare.h"
#include "log/log.h"

volatile int ServerEngine::signal_sys = -1;

ServerEngine::ServerEngine(utrade::pandora::io_service_pool& pool)
{
    data_receiver_ = boost::make_shared<DataReceive>(pool);
    data_processer_ = boost::make_shared<DataProcess>(pool, data_receiver_.get());
    front_server_ = boost::make_shared<FrontServer>(pool, data_processer_.get());        
}

ServerEngine::~ServerEngine()
{
}

void ServerEngine::launch()
{
    LOG->start();

    data_receiver_->launch();
    data_processer_->launch();
    front_server_->launch();
}

void ServerEngine::release()
{
    data_receiver_->release();
    data_processer_->release();
    front_server_->release();
}

void ServerEngine::signal_handler(int signum)
{
//    UT_LOG_INFO(WORMHOLE_LOGGER, "[ServerEngine], signal_handler " << signum);
    printf("\n[ServerEngine], signal_handler:%d \n", signum);
    signal_sys = signum;
    // 释放资源
    SERVER_EENGINE->release();
    // 退出
    exit(0);
}