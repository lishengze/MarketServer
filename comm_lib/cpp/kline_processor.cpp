#include "kline_processor.h"
#include "base/cpp/util.h"

#include "comm_log.h"

COMM_NAMESPACE_START

KlineProcessor::~KlineProcessor()
{

}

bool KlineProcessor::check_kline(KlineData& kline)
{
    try
    {
        COMM_LOG_INFO(kline.str());

        if (!is_kline_valid(kline)) return false;

        if (last_kline_map_.find(kline.symbol) == last_kline_map_.end()
        || last_kline_map_[kline.symbol].find(kline.exchange) == last_kline_map_[kline.symbol].end())
        {
            last_kline_map_[kline.symbol][kline.exchange] = kline;
        }
        else if (kline.index < last_kline_map_[kline.symbol][kline.exchange].index)
        {
            return false;
        }
        else
        {
            last_kline_map_[kline.symbol][kline.exchange] = kline;
        }

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
    return false;
}

void KlineProcessor::on_kline(KlineData& kline)
{
    try
    {
        if (!engine_ || !check_kline(kline)) return;

        engine_->on_kline(kline);             
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

COMM_NAMESPACE_END