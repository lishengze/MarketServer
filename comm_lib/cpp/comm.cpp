#include "comm.h"
#include "comm_log.h"

#include "kafka_server.h"
#include "json_serializer.h"

COMM_NAMESPACE_START

Comm::Comm(string server_address,
            NET_TYPE net_type,
            SERIALIZE_TYPE serialize_type,
            QuoteSourceCallbackInterface* depth_engine,
            QuoteSourceCallbackInterface* kline_engine ,
            QuoteSourceCallbackInterface* trade_engine)
{
    if (serialize_type == SERIALIZE_TYPE::JSON)
    {
        serializer_ = new JsonSerializer(depth_engine, kline_engine, trade_engine);
    }

    if (net_type == NET_TYPE::KAFKA)
    {
        net_server_ = new KafkaServer(server_address, serializer_);
    }
}

Comm::Comm(string server_address, 
            NET_TYPE net_type,
            SERIALIZE_TYPE serialize_type,
            QuoteSourceCallbackInterface* engine_center )
{
    if (serialize_type == SERIALIZE_TYPE::JSON)
    {
        serializer_ = new JsonSerializer(engine_center);
    }

    if (net_type == NET_TYPE::KAFKA)
    {
        net_server_ = new KafkaServer(server_address, serializer_);
    }
}

Comm::~Comm()
{
    if (!net_server_)
    {
        delete net_server_;
    }

    if (!serializer_)
    {
        delete serializer_;
    }
}

void Comm::launch()
{
    try
    {
        net_server_->launch();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void Comm::set_meta(const MetaType kline_meta,
                const MetaType depth_meta,
                const MetaType trade_meta)
{
    try
    {
        net_server_->set_meta(kline_meta, depth_meta, trade_meta);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}                

void Comm::set_kline_meta(const MetaType meta)
{
    try
    {
        net_server_->set_kline_meta(meta);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void Comm::set_depth_meta(const MetaType meta)
{
    try
    {
        net_server_->set_depth_meta(meta);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

void Comm::set_trade_meta(const MetaType meta)
{
    try
    {
        net_server_->set_trade_meta(meta);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}




COMM_NAMESPACE_END