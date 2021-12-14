#include "trade_processor.h"
#include "global_declare.h"

void TradeProcessor::process(TradeData& ori_trade)
{
    try
    {
        if (symbol_config_.find(ori_trade.symbol) == symbol_config_.end()
        || ori_trade.time == 0) return;

        const SMixerConfig& cur_config = symbol_config_[ori_trade.symbol];

        TradeData trade{std::move(ori_trade)};
        trade.exchange = AGGREGATE_EXCHANGE;

        if (last_trade_map_.find(ori_trade.symbol) == last_trade_map_.end())
        {
            trade.price.scale(cur_config.precise);
            trade.volume.scale(cur_config.vprecise);

            last_trade_map_.emplace(std::make_pair(trade.symbol, trade));
        }
        else
        {
            if (trade.time < last_trade_map_[trade.symbol].time) return;

            trade.price.scale(cur_config.precise);
            trade.volume.scale(cur_config.vprecise);
            last_trade_map_[ori_trade.symbol] = trade;
        }

        engine_->on_trade(trade);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

