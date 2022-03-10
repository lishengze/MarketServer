#include "rpc.h"
#include "../Log/log.h"

void BaseRPC::start()
{
    try
    {
        start_monitor_request();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void BaseRPC::process()
{
    try
    {
        if (is_first_process_)
        {
            create_rpc_for_next_client();
            is_first_process_ = false;
        }
        proceed();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void BaseRPC::create_rpc_for_next_client()
{
    try
    {
        BaseRPC* new_rpc = spawn();
        new_rpc->start();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

BaseRPC* BaseRPC::spawn()
{
    try
    {
        BaseRPC* result = new BaseRPC(cq_, service_);

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return nullptr;
}

void RequestTradeDataRPC::start_monitor_request()
{
    try
    {
        service_->RequestRequestTradeData(&context_, &request_info_, &responder_, cq_, cq_, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

BaseRPC* RequestTradeDataRPC::spawn()
{
    try
    {
        BaseRPC* result = new RequestTradeDataRPC(cq_, service_);

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return nullptr;
    
}