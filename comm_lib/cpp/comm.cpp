#include "comm.h"
#include "comm_log.h"

COMM_NAMESPACE_START

Comm::Comm(string server_address,
            QuoteSourceCallbackInterface* depth_engine ,
            QuoteSourceCallbackInterface* kline_engine ,
            QuoteSourceCallbackInterface* trade_engine)
{
    kafka_sptr_ = boost::make_shared<KafkaServer>(depth_engine, kline_engine, trade_engine);
}

void Comm::launch()
{
    try
    {
        kafka_sptr_->launch();
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
        kafka_sptr_->set_meta(kline_meta, depth_meta, trade_meta);
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
        kafka_sptr_->set_kline_meta(meta);
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
        kafka_sptr_->set_depth_meta(meta);
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
        kafka_sptr_->set_trade_meta(meta);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}




COMM_NAMESPACE_END