#include "trade_aggregater.h"
#include "Log/log.h"

void TradeAggregater::on_trade(TradeData& ori_trade)
{
    try
    {
        if (symbol_config_.find(ori_trade.symbol) == symbol_config_.end()) return;

        // if(ori_trade.symbol == "BTC_USDT") LOG_INFO(ori_trade.str());

        // LOG->record_input_info(ori_trade.meta_str(), ori_trade);

        TradeData trade{std::move(ori_trade)};
        trade.exchange = MIX_EXCHANGE_NAME;        

        // LOG->record_output_info(ori_trade.meta_str(), ori_trade);
        
        p_comm_->publish_trade(trade);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}