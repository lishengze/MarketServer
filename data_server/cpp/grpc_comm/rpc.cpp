#include "rpc.h"
#include "../Log/log.h"
#include "../global_declare.h"
#include "server.h"

#include "pandora/util/time_util.h"

uint64 BaseRPC::RPC_ID = 0;

void BaseRPC::start()
{
    try
    {
        LOG_INFO("start");
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
        if (is_first_connect_)
        {
            LOG_INFO(rpc_info() + " first connect-------------\n");
            create_rpc_for_next_client();
            is_first_connect_ = false;
        }

        if (is_inner_cq_event_)
        {
            is_inner_cq_event_ = false;
            LOG_INFO(rpc_info() + " inner cq event");
            return;
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
        LOG_INFO(rpc_info() + " create_rpc_for_next_client;");
        BaseRPC* new_rpc = spawn();
        new_rpc->register_server(server_);
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
        BaseRPC* result = new BaseRPC(service_, cq_, "BaseRPC");

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
        LOG_INFO("RequestTradeDataRPC start_monitor_request");
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

void RequestTradeDataRPC::proceed()
{
    try
    {
       LOG_INFO(rpc_info() + " proceed");

       LOG_INFO("RequestTrade: " + request_info_.exchange() + "." + request_info_.symbol() 
                + ": " + std::to_string(request_info_.time()));


        TradeData trade_data;

        ReqTradeInfo req_trade(request_info_);

        server_->get_req_trade_info(req_trade, trade_data);


        reply_info_.set_exchange(request_info_.exchange());

        reply_info_.set_symbol(request_info_.symbol());

        reply_info_.set_time(utrade::pandora::NanoTime());

        reply_info_.mutable_price()->set_precise(8);
        reply_info_.mutable_price()->set_value(trade_data.price.get_value());

        reply_info_.mutable_volume()->set_precise(8);
        reply_info_.mutable_volume()->set_value(trade_data.volume.get_value());           

        is_inner_cq_event_ = true;        

        responder_.Finish(reply_info_, grpc::Status::OK, this);       
        is_finished_ = true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void GetTradeStreamDataRPC::start_monitor_request()
{
    try
    {
        LOG_INFO(rpc_info() + " start_monitor_request");
        service_->RequestGetStreamTradeData(&context_, &request_info_, &responder_, cq_, cq_, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

BaseRPC* GetTradeStreamDataRPC::spawn()
{
    try
    {
        BaseRPC* result = new GetTradeStreamDataRPC(cq_, service_);

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return nullptr;
    
}

void GetTradeStreamDataRPC::proceed()
{
    try
    {
       LOG_INFO(rpc_info() + " proceed");

       LOG_INFO("RequestTrade: " + request_info_.exchange() + "." + request_info_.symbol() 
                + ": " + std::to_string(request_info_.time()));

        reply_info_.set_exchange(request_info_.exchange());

        reply_info_.set_symbol(request_info_.symbol());

        reply_info_.set_time(utrade::pandora::NanoTime());

        reply_info_.mutable_price()->set_precise(8);
        reply_info_.mutable_price()->set_value(rpc_id);

        reply_info_.mutable_volume()->set_precise(8);
        reply_info_.mutable_volume()->set_value(rpc_id);           

        responder_.Write(reply_info_, this);


        is_inner_cq_event_ = true;


        responder_.Finish(grpc::Status::OK, this);       
        is_finished_ = true;
        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}