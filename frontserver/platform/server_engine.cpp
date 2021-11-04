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

string get_signum_str(int signum)
{
    string result ="unknown signum: " + std::to_string(signum);

    switch (signum)
    {
        case SIGTERM:
            result = "SIGTERM";
            break;
        case SIGINT:
            result = "SIGINT";
            break;

        case SIGHUP:
            result = "SIGHUP";
            break;

        case SIGQUIT:
            result = "SIGQUIT";
            break;

        case SIGKILL:
            result = "SIGKILL";
            break;
        case SIGABRT:
            result = "SIGABRT";
            break;
        case SIGSEGV:
            result = "SIGSEGV";
            break;                                            
        default:
            break;
    }
    return result;
}

void ServerEngine::signal_handler(int signum)
{
//    UT_LOG_INFO(WORMHOLE_LOGGER, "[ServerEngine], signal_handler " << signum);
    // printf("\n[ServerEngine], signal_handler:%d \n", signum);

    LOG_ERROR("[ServerEngine], signal_handler: " + get_signum_str(signum));
    signal_sys = signum;
    // 释放资源
    SERVER_EENGINE->release();
    // 退出
    exit(-1);
}