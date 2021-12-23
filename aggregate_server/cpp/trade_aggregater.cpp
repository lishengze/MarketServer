#include "trade_aggregater.h"
#include "Log/log.h"

void TradeAggregater::on_trade(TradeData& ori_trade)
{
    try
    {
        if (symbol_config_.find(ori_trade.symbol) == symbol_config_.end()) return;

        LOG_INFO(ori_trade.str());

        TradeData trade{std::move(ori_trade)};
        trade.exchange = MIX_EXCHANGE_NAME;        

        p_comm_->publish_trade(trade);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}