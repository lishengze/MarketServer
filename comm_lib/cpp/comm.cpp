#include "comm.h"
#include "comm_log.h"

#include "kafka_server.h"
#include "json_serializer.h"
#include "protobuf_serializer.h"
#include "util.h"

COMM_NAMESPACE_START



Comm::Comm(string server_address,
            QuoteSourceCallbackInterface* depth_engine,
            QuoteSourceCallbackInterface* kline_engine,
            QuoteSourceCallbackInterface* trade_engine,
            NET_TYPE net_type,
            SERIALIZE_TYPE serialize_type)
{
    init_log();

    if (serialize_type == SERIALIZE_TYPE::JSON)
    {
        serializer_ = new JsonSerializer(depth_engine, kline_engine, trade_engine);
    }
    else if (serialize_type ==  SERIALIZE_TYPE::PROTOBUF)
    {
        serializer_ = new ProtobufSerializer(depth_engine, kline_engine, trade_engine);
    }
    else
    {
        COMM_LOG_ERROR("Unknown Protobuf Type");
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
    init_log();
    
    if (serialize_type == SERIALIZE_TYPE::JSON)
    {
        serializer_ = new JsonSerializer(engine_center);
    }
    else if (serialize_type ==  SERIALIZE_TYPE::PROTOBUF)
    {
        serializer_ = new ProtobufSerializer(engine_center);
    }
    else
    {
        COMM_LOG_ERROR("Unknown Protobuf Type");
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

        COMM_LOG_INFO("Comm launch End!");        
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

void Comm::publish_depth(const SDepthQuote& quote)
{
    try
    {
        net_server_->publish_depth(quote);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }  
}

void Comm::publish_kline(const KlineData& kline)
{
    try
    {
        net_server_->publish_kline(kline);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }  
}

void Comm::publish_trade(const TradeData& trade)
{
    try
    {
       net_server_->publish_trade(trade);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}


COMM_NAMESPACE_END