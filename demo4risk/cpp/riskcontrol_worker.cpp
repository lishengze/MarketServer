#include "riskcontrol_worker.h"
#include "risk_controller_config.h"
#include "grpc_server.h"
#include "converter.h"
#include "updater_configuration.h"

#include "updater_quote.h"
#include "Log/log.h"
#include "util/tool.h"


bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void _filter_depth_by_watermark(SInnerQuote& src, const SDecimal& watermark, bool is_ask, PipelineContent& ctx)
{
    
    if( is_ask )
    {
        map<SDecimal, SInnerDepth>& src_depths = src.asks;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一
        std::list<SDecimal> delete_price;

        for( auto v = src_depths.begin() ; v != src_depths.end() ; v++)
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;

            if( price <= watermark) 
            {
                // 过滤价位
                for( const auto& v2 : depth.exchanges )
                {
                    volumes[v2.first] += v2.second;
                }

                delete_price.push_back(price);

                if (WATERMARK_RISKCTRL_OPEN && src.symbol == CONFIG->test_symbol)
                {
                    LOG_DEBUG("watermark filter ask " + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
                }
            } 
            else 
            {
                break;
            }
        }

        if( !patched ) 
        {
            patched = true;            
            SInnerDepth fake;
            for( const auto &v : volumes ) {
                fake.exchanges[v.first] = v.second;
            }
            fake.set_total_volume();

            if (fake.total_volume.get_value() != 0)
            {
                SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];
                SDecimal new_price = watermark + symbol_config.MinChangePrice;
                src_depths[new_price] = fake;

                stringstream s_s;
                s_s << "" << src.symbol << ", "
                    << "add new price: " << new_price.get_value() << ", "
                    << "volume: " << src_depths[new_price].total_volume.get_value() << ", "
                    << symbol_config.MinChangePrice
                    << ", is_ask: " << is_ask << "\n";
                    
                if (src.symbol == CONFIG->test_symbol)
                {
                    LOG_DEBUG(s_s.str());
                }                        
            }
        } 

        for (auto price:delete_price)
        {
            src_depths.erase(price);
        }
    }
    else // For Bid
    {
        map<SDecimal, SInnerDepth>& src_depths = src.bids;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一
        std::list<SDecimal> delete_price;

        for( auto v = src_depths.rbegin() ; v != src_depths.rend() ; v++)
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;
            if( price >= watermark ) {
                // 过滤价位
                for( const auto& v2 : depth.exchanges )
                {
                    volumes[v2.first] += v2.second;
                }

                delete_price.push_back(price);

                if (WATERMARK_RISKCTRL_OPEN && src.symbol == CONFIG->test_symbol)
                {
                    LOG_DEBUG("watermark filter bid " + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
                }
            } 
            else 
            {
                break;
            }
        }

        // 保留价位
        if( !patched ) {
            patched = true;
            
            SInnerDepth fake;
            for( const auto &v : volumes ) {
                fake.exchanges[v.first] = v.second;
            }
            fake.set_total_volume();

            if (fake.total_volume.get_value() != 0)
            {
                SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];

                SDecimal new_price = watermark - symbol_config.MinChangePrice;

                src_depths[new_price] = fake;

                stringstream s_s;
                s_s << src.symbol << ", "
                    << "add new price: " << new_price.get_str_value() << ", "
                    << "volume: " << src_depths[new_price].total_volume.get_str_value() << ", "
                    << symbol_config.MinChangePrice
                    << ", is_ask: " << is_ask << "\n";
                    
                if (src.symbol == CONFIG->test_symbol)
                {
                    LOG_DEBUG(s_s.str());
                }
            }
        } 


        for (auto price:delete_price)
        {
            src_depths.erase(price);
        }
    }
}

void _filter_by_watermark(SInnerQuote& src, const SDecimal& watermark, PipelineContent& ctx)
{
    _filter_depth_by_watermark(src, watermark, true, ctx);
    _filter_depth_by_watermark(src, watermark, false, ctx);
}

SDecimal get_bias_decimal(const SDecimal& ori_data, int bias_kind, double bias_value)
{
    SDecimal result = ori_data;
    try
    {
        if (bias_kind == 1)
        {
            double rate = (1+bias_value) < 0 ? 0 : (1+bias_value);

            result = ori_data * rate;
        }
        else if (bias_kind == 2)
        {
            result = (ori_data+bias_value) < 0 ? 0 : (ori_data+bias_value);
        }
        else
        {
            LOG_ERROR("Unknown BiasKind: " + std::to_string(bias_kind));
        }

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return result;
}

void _calc_depth_bias(const vector<pair<SDecimal, SInnerDepth>>& depths, MarketRiskConfig& config, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    double price_bias = config.PriceOffset;
    double volume_bias = config.AmountOffset;

    map<SDecimal, SInnerDepth> result;

    for( const auto& v: depths )
    {
        SDecimal scaledPrice = v.first;

        // double scaledPrice = v.first.get_value();

        if (is_ask)
        {
            scaledPrice = get_bias_decimal(scaledPrice, config.PriceOffsetKind, config.PriceOffset);
        }
        else
        {
            scaledPrice = get_bias_decimal(scaledPrice, config.PriceOffsetKind, config.PriceOffset*-1);
        }

        // // dst[v.first].mix_exchanges(v.second, 0);

        // SDecimal decimal_value(scaledPrice);

        SDecimal bias_volume = get_bias_decimal(v.second.total_volume, config.AmountOffsetKind, config.AmountOffset*-1);

        result[scaledPrice].total_volume = bias_volume;

        for (auto exchange_iter:v.second.exchanges)
        {
            result[scaledPrice].exchanges[exchange_iter.first] = get_bias_decimal(exchange_iter.second, config.AmountOffsetKind, config.AmountOffset*-1);
        }
    }

    dst.swap(result);
}

void _calc_depth_fee(const vector<pair<SDecimal, SInnerDepth>>& depths, SymbolConfiguration& config, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    map<SDecimal, SInnerDepth> result;

    for( const auto& v: depths )
    {
        SDecimal scaledPrice = v.first;

        // double scaledPrice = v.first.get_value();

        if (config.FeeKind == 1)
        {
            if( is_ask ) 
            {
                scaledPrice *= ( 1 + config.TakerFee);
            } 
            else 
            {
                if( config.TakerFee < 1 )
                    scaledPrice *= ( 1 - config.TakerFee);
                else
                    scaledPrice = 0;
            }
        }
        else if (config.FeeKind == 2)
        {
            if( is_ask ) 
            {
                scaledPrice += config.TakerFee;
            } 
            else 
            {
                scaledPrice -= config.TakerFee;
            }

            scaledPrice = scaledPrice > 0 ? scaledPrice : 0;
        }

        SDecimal decimal_value(scaledPrice);
        result[decimal_value] = v.second;
    }

    dst.swap(result);
}

void _calc_depth_fee(const vector<pair<SDecimal, SInnerDepth>>& depths, map<string, HedgeConfig>& config_exchange_map, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    map<SDecimal, SInnerDepth> result;

    for(auto& v: depths )
    {
        
        const SInnerDepth& inner_depth = v.second;

        for (auto iter:inner_depth.exchanges)
        {
            SDecimal ori_price  = v.first;
            const string& exchange = iter.first;

            if (config_exchange_map.find(exchange) == config_exchange_map.end())
            {
                LOG_ERROR("Need Hedge Config For " + exchange);
                continue;
            }
            HedgeConfig& config = config_exchange_map[exchange];

            if (config.FeeKind == 1)
            {
                if( is_ask ) 
                {
                    ori_price *= ( 1 + config.TakerFee);
                } 
                else 
                {
                    if( config.TakerFee < 1 )
                        ori_price *= ( 1 - config.TakerFee);
                    else
                        ori_price = 0;
                }
            }
            else if (config.FeeKind == 2)
            {
                if( is_ask ) 
                {
                    ori_price += config.TakerFee;
                } 
                else 
                {
                    ori_price -= config.TakerFee;
                }
                ori_price = ori_price > 0?ori_price : 0;
            }

            if (result.find(ori_price) == result.end())
            {
                SInnerDepth new_depth;
                new_depth.exchanges[exchange] = iter.second; 
                new_depth.set_total_volume();
                result[ori_price] = new_depth;
            }
            else
            {
                if (result[ori_price].exchanges.find(exchange) != result[ori_price].exchanges.end())
                {
                    result[ori_price].exchanges[exchange] += iter.second;
                }
                else
                {
                    result[ori_price].exchanges[exchange] = iter.second;
                }
                result[ori_price].set_total_volume();
            }

        }

        // double scaledPrice = v.first.get_value();
        // SDecimal decimal_value(scaledPrice);
        // result[decimal_value] = v.second;
    }

    dst.swap(result);
}

void set_depth_presion(map<SDecimal, SInnerDepth>& depth, int price_precision, int amount_precision)
{
    try
    {
        map<SDecimal, SInnerDepth> new_depth;

        for (map<SDecimal, SInnerDepth>::iterator iter = depth.begin(); iter!=depth.end(); ++iter)
        {
            SDecimal new_price = iter->first;
            SInnerDepth new_atom_depth = iter->second;

            new_price.scale(price_precision);            

            // for (auto iter:new_atom_depth.exchanges)
            // {
            //     iter.second.scale(amount_precision);
            // }
            // new_atom_depth.set_total_volume();

            new_atom_depth.total_volume.scale(amount_precision);
            new_depth[new_price] = new_atom_depth;
        }

        depth.swap(new_depth);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

// For Fee
SInnerQuote& FeeWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    try
    {
        // LOG_DEBUG(quote_str(src, 3));

        // if( ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end() ) 
        // {

        //     if (src.symbol == CONFIG->test_symbol && CONFIG->fee_risk_ctrl_open_ )
        //     {
        //         LOG_DEBUG("\nBefore FeeWorker: " + quote_str(src, 3));
        //         LOG_DEBUG(ctx.params.symbol_config[src.symbol].fee_info());
        //     } 

        //     SInnerQuote tmp;
        //     vector<pair<SDecimal, SInnerDepth>> depths;
        //     src.get_asks(depths);
        //     _calc_depth_fee(depths, ctx.params.symbol_config[src.symbol],  true, tmp.asks);
        //     src.asks.swap(tmp.asks);
        //     src.get_bids(depths);
        //     _calc_depth_fee(depths, ctx.params.symbol_config[src.symbol],  false, tmp.bids);
        //     src.bids.swap(tmp.bids);
        // }
        // else
        // {
        //     LOG_WARN("symbol_config Can't Find " + src.symbol);
        // }

        if( ctx.params.hedge_config.find(src.symbol) != ctx.params.hedge_config.end() ) 
        {

            if (src.symbol == CONFIG->test_symbol && CONFIG->fee_risk_ctrl_open_ )
            {
                LOG_DEBUG("\nBefore FeeWorker: " + quote_str(src, 3));

                for (auto iter: ctx.params.hedge_config[src.symbol])
                {
                    LOG_DEBUG("\nHedgeConfig: \n" + iter.second.str());
                }
            } 

            SInnerQuote tmp;
            vector<pair<SDecimal, SInnerDepth>> depths;
            src.get_asks(depths);
            _calc_depth_fee(depths, ctx.params.hedge_config[src.symbol],  true, tmp.asks);
            src.asks.swap(tmp.asks);
            src.get_bids(depths);
            _calc_depth_fee(depths, ctx.params.hedge_config[src.symbol],  false, tmp.bids);
            src.bids.swap(tmp.bids);
        }
        else
        {
            LOG_WARN("hedge_config Can't Find " + src.symbol);
        }

        if (src.symbol == CONFIG->test_symbol && CONFIG->fee_risk_ctrl_open_ )
        {
            LOG_DEBUG("\nAfter FeeWorker: " + quote_str(src, 3));
        } 

        // if (src.symbol == "BTC_USD")
        // {
        //     LOG_DEBUG(ctx.params.market_risk_config[src.symbol].desc());
        //     LOG_DEBUG("\nAfter QuoteBiasWorker: " + " " 
        //                 + src.symbol + " " + quote_str(src, 2));
        // } 
                    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return src;
}

// For Account
void AccountAjdustWorker::set_snap(const SInnerQuote& quote) 
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
        // 计算币种出现的次数
        if( symbols_.find(quote.symbol) == symbols_.end() ) {
            symbols_.insert(quote.symbol);
            string sell_currency, buy_currency;
            if( getcurrency_from_symbol(quote.symbol, sell_currency, buy_currency) ) {
                currency_count_[sell_currency] += 1;
                currency_count_[buy_currency] += 1;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool AccountAjdustWorker::get_currency(const SInnerQuote& quote, string& sell_currency, int& sell_count, string& buy_currency, int& buy_count) const
{
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
    if( !getcurrency_from_symbol(quote.symbol, sell_currency, buy_currency) )
        return false;
    auto iter = currency_count_.find(sell_currency);
    if( iter == currency_count_.end() )
        return false;
    sell_count = iter->second; // 卖币种出现次数
    auto iter2 = currency_count_.find(buy_currency);
    if( iter2 == currency_count_.end() )
        return false;
    buy_count = iter->second; // 买币种出现次数
    return true;
}

SInnerQuote& AccountAjdustWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    // set_snap(src);

    if (src.symbol == CONFIG->test_symbol && CONFIG->account_risk_ctrl_open_ )
    {
        LOG_DEBUG("\nBefore AccountAjdustWorker: " + quote_str(src, 3));
    } 


    // 获取配置
    map<TExchange, HedgeConfig>& hedge_config_map = ctx.params.hedge_config[src.symbol];

    // // 获取币种出现次数
    // string sell_currency, buy_currency;
    // int sell_count, buy_count;
    // if( !get_currency(src, sell_currency, sell_count, buy_currency, buy_count) )
    //     return src;

    // // 动态调整每个品种的资金分配量
    // unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    // ctx.params.account_config.get_hedge_amounts(sell_currency, hedge_percent / sell_count, sell_total_amounts);
    // ctx.params.account_config.get_hedge_amounts(buy_currency, hedge_percent / buy_count, buy_total_amounts);

    string sell_currency, buy_currency;
    // int sell_count, buy_count;
    if(!getcurrency_from_symbol(src.symbol, sell_currency,  buy_currency))
    {
        LOG_WARN("Get Currency From Symbol: " + src.symbol + " Failed!");
        return src;
    }
    
    // check_exchange_volume(src);
    // 动态调整每个品种的资金分配量

    std::stringstream s_s;

    s_s << ctx.params.account_config.hedage_account_str();

    s_s << "hedge_config: \n";
    for (auto iter:hedge_config_map)
    {
        s_s << iter.second.str();
    }
    s_s << "\n";
    
    unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    ctx.params.account_config.get_hedge_amounts(sell_currency, hedge_config_map, sell_total_amounts, false);
    ctx.params.account_config.get_hedge_amounts(buy_currency, hedge_config_map, buy_total_amounts, true);

    
    s_s << "Before Risk \nsell_total_amounts " << sell_currency << "\n";
    for (auto iter:sell_total_amounts)
    {
        s_s << iter.first << ": " << std::to_string(iter.second) << "\n";
    }

    s_s << "buy_total_amounts " << buy_currency << "\n";
    for (auto iter:buy_total_amounts)
    {
        s_s << iter.first << ": " << std::to_string(iter.second) << "\n";
    }

    if (src.symbol == CONFIG->test_symbol && CONFIG->account_risk_ctrl_open_)
    {
        LOG_DEBUG(s_s.str());
    }

    s_s.clear();

    // 卖的方向，从bids列表中控制量;
    bool is_ctrl_over = false;
    for( auto iter = src.bids.rbegin() ; iter != src.bids.rend() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        // SDecimal ori_total_volume = depth.total_volume;
        depth.total_volume = 0;

        if (is_ctrl_over) continue;
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second;
            double remain_amount = sell_total_amounts[exchange];

            s_s << "bid " << exchange << " p: " + iter->first.get_str_value() + " " 
                << " need_amount: " + need_amount.get_str_value()  
                << ", remain_amount: " + std::to_string(remain_amount) << "\n";
            
            if(remain_amount < need_amount.get_value())
            {
                if (remain_amount <= 0) iter2->second = 0;
                else iter2->second = remain_amount;                
                depth.total_volume += iter2->second;
                is_ctrl_over = true;
                sell_total_amounts[exchange] = 0;
                break;
            } 
            else 
            {
                sell_total_amounts[exchange] -= need_amount.get_value();
                depth.total_volume += iter2->second;
            }
        }
    }

    for( auto iter = src.bids.begin() ; iter != src.bids.end() ; ) 
    {
        SInnerDepth& depth = iter->second;
        if( depth.total_volume.is_zero() ) {
            src.bids.erase(iter++);
        } else {
            iter++;
        }
    }
    
    // 买的方向，从asks列表中控制量;
    is_ctrl_over = false;
    for( auto iter = src.asks.begin() ; iter != src.asks.end() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        depth.total_volume = 0;
        
        if (is_ctrl_over) continue;
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second * iter->first.get_value();
            double remain_amount = buy_total_amounts[exchange];
            
            s_s << "ask " << exchange << " p: " + iter->first.get_str_value() + " " 
                << " need_amount: " + need_amount.get_str_value()  
                << ", remain_amount: " + std::to_string(remain_amount) << "\n";

            if( remain_amount < need_amount.get_value() ) 
            {
                if (remain_amount <= 0) iter2->second = 0;
                else iter2->second = remain_amount / iter->first.get_value();

                depth.total_volume += iter2->second;
                is_ctrl_over = true;
                buy_total_amounts[exchange] = 0;
                break;
            } 
            else 
            {
                buy_total_amounts[exchange] -= need_amount.get_value();
                depth.total_volume += iter2->second;
            }
        }
    }

    for( auto iter = src.asks.begin() ; iter != src.asks.end() ; ) 
    {
        SInnerDepth& depth = iter->second;
        if( depth.total_volume.is_zero() ) {
            src.asks.erase(iter++);
        } else {
            iter++;
        }
    }

    // s_s << "\nAfter Risk sell_total_amounts " << sell_currency << "\n";
    // for (auto iter:sell_total_amounts)
    // {
    //     s_s << iter.first << ": " << iter.second << "\n";
    // }

    // s_s << "buy_total_amounts " << buy_currency << "\n";
    // for (auto iter:buy_total_amounts)
    // {
    //     s_s << iter.first << ": " << iter.second << "\n";
    // }


    // if (src.symbol == CONFIG->test_symbol)
    // {
    //     LOG_DEBUG(s_s.str());
    // }

    // if (src.symbol == "BTC_USDT")
    // {
    //     LOG_DEBUG("\nAfter AccountAjdustWorker: " + " " 
    //                 + src.symbol + " " + quote_str(src, 8));
    // } 
        
    if (src.symbol == CONFIG->test_symbol && CONFIG->account_risk_ctrl_open_)
    {
        LOG_DEBUG("\nAfter AccountAjdustWorker: " + quote_str(src, 3));
    } 

    return src;
}

void inner_depth_minus_amount(SInnerDepth& inner_depth, SDecimal amount)
{
    try
    {
        if (inner_depth.total_volume > amount)
        {
            inner_depth.total_volume -= amount;

            for (auto& iter:inner_depth.exchanges)
            {
                if (iter.second > amount)
                {
                    iter.second -= amount;
                    break;
                }
                else
                {
                    iter.second = 0;
                    amount -= iter.second;
                }
            }
        }
        else
        {
            inner_depth.total_volume = 0;

            for (auto& iter:inner_depth.exchanges)
            {
                iter.second = 0;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

// For Order
SInnerQuote& OrderBookWorker::process(SInnerQuote& src, PipelineContent& ctx)
{        
    try
    {
        if (src.symbol == CONFIG->test_symbol && CONFIG->order_risk_ctrl_open_ )
        {
            LOG_DEBUG("\nBefore OrderBookWorker: " + quote_str(src, 3));
        } 

        if (ctx.params.hedage_order_info.find(src.symbol) != ctx.params.hedage_order_info.end())
        {
            HedgeInfo& hedage_order_info = ctx.params.hedage_order_info[src.symbol];

            if (src.symbol == CONFIG->test_symbol && CONFIG->order_risk_ctrl_open_ )
            {
                LOG_DEBUG(hedage_order_info.str());
            } 

            if (hedage_order_info.ask_amount > 0)
            {
                double ask_amount = hedage_order_info.ask_amount;

                std::vector<SDecimal> delete_price;

                for (map<SDecimal, SInnerDepth>::reverse_iterator iter = src.bids.rbegin(); iter != src.bids.rend(); iter++)
                {
                    if (iter->second.total_volume > ask_amount)
                    {
                        // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                        //         + ", amount: " + iter.second.total_volume.get_str_value() 
                        //         + ", minus " + std::to_string(bid_amount));

                        // iter->second.total_volume -= ask_amount;

                        inner_depth_minus_amount(iter->second, ask_amount);
                        break;
                    }
                    else
                    {
                        // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                        //         + ", is deleted, delete amount: " + iter.second.total_volume.get_str_value() 
                        //         + ", bid_amount " + std::to_string(bid_amount));

                        ask_amount -= iter->second.total_volume.get_value();
                        // iter->second.total_volume = 0;
                        inner_depth_minus_amount(iter->second, ask_amount);
                        delete_price.push_back(iter->first);
                    }
                }

                for (auto price: delete_price)
                {
                    src.bids.erase(price);
                }         
            }

            if (hedage_order_info.bid_amount > 0)
            {
                double bid_amount = hedage_order_info.bid_amount;

                std::vector<SDecimal> delete_price;

                for ( map<SDecimal, SInnerDepth>::iterator iter=src.asks.begin(); iter != src.asks.end(); ++iter)
                {
                    if (iter->second.total_volume > bid_amount)
                    {
                        // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                        //         + ", amount: " + iter->second.total_volume.get_str_value() 
                        //         + ", minus " + std::to_string(ask_amount));
                        // iter->second.total_volume -= bid_amount;

                        inner_depth_minus_amount(iter->second, bid_amount);

                        break;
                    }
                    else
                    {
                        // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                        //         + ", is deleted, delete amount: " + iter->second.total_volume.get_str_value() 
                        //         + ", ask_amount " + std::to_string(ask_amount));
                        bid_amount -= iter->second.total_volume.get_value();

                        inner_depth_minus_amount(iter->second, bid_amount);
                        
                        // iter->second.total_volume = 0;
                        delete_price.push_back(iter->first);
                    }
                }

                for (auto price: delete_price)
                {
                    src.asks.erase(price);
                }
            }
        }
        else
        {
            if (src.symbol == CONFIG->test_symbol && CONFIG->order_risk_ctrl_open_ )
            {
                LOG_DEBUG("No Order Info For " + src.symbol);
            } 
        }

        if (src.symbol == CONFIG->test_symbol && CONFIG->order_risk_ctrl_open_ )
        {
            LOG_DEBUG("\nAfter OrderBookWorker: " + quote_str(src, 3));
        } 


        return src;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return src;
};

// For Bias
SInnerQuote& QuoteBiasWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    try
    {


        if( ctx.params.market_risk_config.find(src.symbol) != ctx.params.market_risk_config.end() ) 
        {

            if (src.symbol == CONFIG->test_symbol && CONFIG->bias_risk_ctrl_open_ )
            {
                LOG_DEBUG("\nBefore QuoteBiasWorker: " + quote_str(src, 3));
                LOG_DEBUG(ctx.params.market_risk_config[src.symbol].desc());
            } 


            SInnerQuote tmp;
            vector<pair<SDecimal, SInnerDepth>> depths;
            src.get_asks(depths);
            _calc_depth_bias(depths, ctx.params.market_risk_config[src.symbol],  true, tmp.asks);
            src.asks.swap(tmp.asks);
            src.get_bids(depths);
            _calc_depth_bias(depths, ctx.params.market_risk_config[src.symbol],  false, tmp.bids);
            src.bids.swap(tmp.bids);
        }
        else
        {
            LOG_WARN("QuoteConfig Can't Find " + src.symbol);
        }

        if (src.symbol == CONFIG->test_symbol && CONFIG->bias_risk_ctrl_open_ )
        {
            LOG_DEBUG("\nAfter QuoteBiasWorker: " + quote_str(src, 3));
        } 


        // if (src.symbol == "BTC_USD")
        // {
        //     LOG_DEBUG(ctx.params.market_risk_config[src.symbol].desc());
        //     LOG_DEBUG("\nAfter QuoteBiasWorker: " + " " 
        //                 + src.symbol + " " + quote_str(src, 2));
        // } 
                    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return src;
}

// For WaterMark
WatermarkComputerWorker::WatermarkComputerWorker() 
{
    worker_name = "WatermarkComputerWorker";
    // thread_run_ = true;
    // thread_loop_ = new std::thread(&WatermarkComputerWorker::_calc_watermark, this);
}

WatermarkComputerWorker::~WatermarkComputerWorker() 
{
    thread_run_ = false;
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void WatermarkComputerWorker::query(map<TSymbol, SDecimal>& watermarks) const
{
    watermarks.clear();
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
    for( const auto& v : watermark_ ) {
        watermarks[v.first] = v.second->watermark;
    }
}

void WatermarkComputerWorker::set_snap(const SInnerQuote& quote) 
{
    // 提取各交易所的买卖一

    unordered_map<TExchange, SDecimal> first_ask, first_bid;
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++ ) {
    
        // LOG_DEBUG("ask price " + iter->first.get_str_value());

        for( const auto& v : iter->second.exchanges ) {
            const TExchange& exchange = v.first;

            //  LOG_DEBUG(exchange + " volume:" + v.second.get_str_value());

            if( first_ask.find(exchange) == first_ask.end() ) {
                first_ask[exchange] = iter->first;
            }
        }    
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {
        // LOG_DEBUG("bid price " + iter->first.get_str_value());
        for( const auto& v : iter->second.exchanges ) {

            //  LOG_DEBUG(exchange + " volume:" + v.second.get_str_value());

            const TExchange& exchange = v.first;
            if( first_bid.find(exchange) == first_bid.end() ) {
                first_bid[exchange] = iter->first;
            }
        }      
    }

    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
    auto iter = watermark_.find(quote.symbol);
    if( iter == watermark_.end() ) {
        SymbolWatermark* obj = new SymbolWatermark();
        obj->asks = first_ask;
        obj->bids = first_bid;
        watermark_[quote.symbol] = obj;
    } else {
        iter->second->asks = first_ask;
        iter->second->bids = first_bid;
    }

    if (WATERMARK_RISKCTRL_OPEN)
    {
        string result = "watermark_ info: ";
        for (auto iter:watermark_)
        {
            result += iter.first + ",";
        }
        // LOG_DEBUG(result);
    }
}

bool WatermarkComputerWorker::get_watermark(const string& symbol, SDecimal& watermark) const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };    
    auto iter = watermark_.find(symbol);
    if( iter == watermark_.end() || iter->second->watermark.is_zero() ) {
        return false;
    }
    watermark = iter->second->watermark;
    return true;
};

void WatermarkComputerWorker::_calc_watermark() 
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
        // 计算watermark
        // LOG_DEBUG("_calc_watermark");
        for( auto iter = watermark_.begin() ; iter != watermark_.end() ; ++iter ) 
        {

            // LOG_DEBUG("Calc " + iter->first);
            
            SymbolWatermark* obj= iter->second;
            vector<SDecimal> asks, bids;
            for( auto &v : obj->asks ) { asks.push_back(v.second); }
            for( auto &v : obj->bids ) { bids.push_back(v.second); }
            // 排序
            sort(asks.begin(), asks.end());

            if (iter->first == CONFIG->test_symbol && CONFIG->watermark_risk_ctrl_open_)
            {
                string result = "asks first depth list: ";
                for( const auto& v : asks ) {
                    result += v.get_str_value();
                }                
                LOG_DEBUG(result);
            }


            sort(bids.begin(), bids.end());

            if (iter->first == CONFIG->test_symbol && CONFIG->watermark_risk_ctrl_open_)
            {
                string result = "bids first depth list: ";
                for( const auto& v : bids ) {
                    result += v.get_str_value();
                }             
                LOG_DEBUG(result);   
            }

            if( asks.size() > 0 && bids.size() > 0 ) {
                iter->second->watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;

                if (iter->first == CONFIG->test_symbol && CONFIG->watermark_risk_ctrl_open_)
                {
                    LOG_DEBUG(iter->first + ", ask_median: " + asks[asks.size()/2].get_str_value() + ",bid_median: " + bids[bids.size()/2].get_str_value() 
                                + ", watermark: " + iter->second->watermark.get_str_value());
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("[E] " + e.what());
    }        
}

void WatermarkComputerWorker::thread_func() {

    while( thread_run_ ) {

        // _calc_watermark();

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};
   
SInnerQuote& WatermarkComputerWorker::process(SInnerQuote& src, PipelineContent& ctx)
{

    if (src.symbol == CONFIG->test_symbol && CONFIG->watermark_risk_ctrl_open_ )
    {
        LOG_DEBUG("\nBefore WatermarkComputerWorker: " + quote_str(src, 3));
    } 

    set_snap(src);
    _calc_watermark();
    
    SDecimal watermark;
    get_watermark(src.symbol, watermark);

    if (watermark.get_value() != 0 && ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
    {
        _filter_by_watermark(src, watermark, ctx);
    }

    if (src.asks.size() > 0 && src.bids.size() > 0)
    {
        if (src.asks.begin()->first < src.bids.rbegin()->first)
        {
            LOG_ERROR(src.symbol + " Depth Error ask.begin: " + src.asks.begin()->first.get_str_value()
                      + " < bids.end:" +  src.bids.rbegin()->first.get_str_value());
        }
    }
                
    if (src.symbol == CONFIG->test_symbol && CONFIG->watermark_risk_ctrl_open_ )
    {
        LOG_DEBUG("\nAfter WatermarkComputerWorker: " + quote_str(src, 3));
    } 


    return src;
}

// For Precision
SInnerQuote& PrecisionWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    try
    {
        if (src.symbol == CONFIG->test_symbol && CONFIG->pricesion_risk_ctrl_open_
        && ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
        {
            LOG_DEBUG("\nBefore PrecisionWorker: " + quote_str(src, 3));
            LOG_DEBUG(ctx.params.symbol_config[src.symbol].desc());
        } 


        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
        {
            int price_precision = ctx.params.symbol_config[src.symbol].PricePrecision;
            int amount_precision = ctx.params.symbol_config[src.symbol].AmountPrecision;

            // if (src.symbol == "BTC_USD")
            // {
            //     LOG_DEBUG(quote_str(src, 3));                        
            // }

            set_depth_presion(src.asks, price_precision, amount_precision);
            set_depth_presion(src.bids, price_precision, amount_precision);

            // if (src.symbol == "BTC_USD")
            // {
            //     LOG_DEBUG(ctx.params.symbol_config[src.symbol].desc());        
            //     LOG_DEBUG(quote_str(src, 3));                  
            // }
        }

        if (src.symbol == CONFIG->test_symbol && CONFIG->pricesion_risk_ctrl_open_ )
        {
            LOG_DEBUG("\nAfter PrecisionWorker: " + quote_str(src, 3));
        } 

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return src;
}

SInnerQuote& DefaultWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    for( auto& v : src.asks ) {
        v.second.total_volume = 0;
        for( const auto& v2 : v.second.exchanges ) {
            v.second.total_volume += v2.second;
        }
    }
    for( auto& v : src.bids ) {
        v.second.total_volume = 0;
        for( const auto& v2 : v.second.exchanges ) {
            v.second.total_volume += v2.second;
        }
    }

        // if (src.symbol == "BTC_USD")
        // {
        //     LOG_DEBUG("\nAfter DefaultWorker: " + " " + src.symbol + " " + quote_str(src, 2));
        // } 


    return src;
}


QuotePipeline::QuotePipeline(){
    add_worker(&default_worker_);
}

QuotePipeline::~QuotePipeline(){

}