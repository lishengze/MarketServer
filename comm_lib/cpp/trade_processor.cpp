#include "trade_processor.h"
#include "base/cpp/util.h"
#include "comm_log.h"

COMM_NAMESPACE_START

bool TradeProcessor::check(TradeData& trade)
{
    try
    {
        COMM_LOG_INFO(trade.str());
        
        if (!is_trade_valid(trade)) return false;

        if (last_trade_map_.find(trade.symbol) == last_trade_map_.end()
        || last_trade_map_[trade.symbol].find(trade.exchange) == last_trade_map_[trade.symbol].end())
        {
            last_trade_map_[trade.symbol][trade.exchange] = trade;
        }
        else if (trade.time < last_trade_map_[trade.symbol][trade.exchange].time)
        {
            return false;
        }
        else
        {
            last_trade_map_[trade.symbol][trade.exchange] = trade;
        }
        return true;
    }
    catch(const std::exception& e)
    {
            COMM_LOG_ERROR(e.what());
    }
    return false;
}

void TradeProcessor::on_trade(TradeData& trade)
{
    try
    {
        if (!engine_ || !check(trade)) return;

        engine_->on_trade(trade);        
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

COMM_NAMESPACE_END
