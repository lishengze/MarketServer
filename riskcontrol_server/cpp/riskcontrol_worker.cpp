#include "riskcontrol_worker.h"


bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void _filter_depth_by_watermark(SDepthQuote& src, const SDecimal& watermark, bool is_ask, PipelineContent& ctx)
{
    
    if( is_ask )
    {
        map<SDecimal, SDepth>& src_depths = src.asks;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一

        for( auto v = src_depths.begin() ; v != src_depths.end() ; )
        {
            const SDecimal& price = v->first;
            SDepth& depth = v->second;

            if( price <= watermark) {
                // 过滤价位
                for( const auto& v2 : depth.volume_by_exchanges ){
                    volumes[v2.first] += v2.second;
                }
                src_depths.erase(v++);
                // LOG_DEBUG("watermark filter ask " + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
            } else {
                // 保留价位
                if( !patched ) {
                    patched = true;
                    
                    SDepth fake;
                    for( const auto &v : volumes ) {
                        fake.volume_by_exchanges[v.first] = v.second;
                    }
                    fake.set_total_volume();

                    if (fake.volume.get_value() != 0)
                    {
                        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
                        {
                            SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];

                            SDecimal new_price = watermark + symbol_config.MinChangePrice;

                            src_depths[new_price] = fake;

                            stringstream s_s;
                            s_s << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].volume.get_value() << ", "
                                << "is_ask: " << is_ask << "\n";
                            // LOG_DEBUG(s_s.str());
                        }
                        else
                        {                            
                            depth.mix_exchanges(fake, 0, 1);
                        }
                    }
                    
                    // 
                } else {
                    break;
                }
                v++;
            }
        }
    }
    else // For Bid
    {
        map<SDecimal, SDepth>& src_depths = src.bids;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一

        for( auto v = src_depths.rbegin() ; v != src_depths.rend() ; )
        {
            const SDecimal& price = v->first;
            SDepth& depth = v->second;
            if( price >= watermark ) {
                // 过滤价位
                for( const auto& v2 : depth.volume_by_exchanges ){
                    volumes[v2.first] += v2.second;
                }
                v = decltype(v)(src_depths.erase( std::next(v).base() ));
                // LOG_DEBUG("watermark filter bid" + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
            } else {
                // 保留价位
                if( !patched ) {
                    patched = true;
                    
                    SDepth fake;
                    for( const auto &v : volumes ) {
                        fake.volume_by_exchanges[v.first] = v.second;
                    }
                    fake.set_total_volume();

                    if (fake.volume.get_value() != 0)
                    {
                        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
                        {
                            SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];

                            SDecimal new_price = watermark - symbol_config.MinChangePrice;

                            src_depths[new_price] = fake;

                            stringstream s_s;
                            s_s << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].volume.get_value() << ", "
                                << "is_ask: " << is_ask << "\n";
                            // LOG_DEBUG(s_s.str());
                        }
                        else
                        {                  
                            depth.mix_exchanges(fake, 0, 1);
                        }
                    }
                } else {
                    break;
                }
                v++;
            }
        }
    }
}

void _filter_by_watermark(SDepthQuote& src, const SDecimal& watermark, PipelineContent& ctx)
{
    _filter_depth_by_watermark(src, watermark, true, ctx);
    _filter_depth_by_watermark(src, watermark, false, ctx);
}

void _calc_depth_bias(const vector<pair<SDecimal, SDepth>>& depths, MarketRiskConfig& config, bool is_ask, map<SDecimal, SDepth>& dst) 
{
    double price_bias = config.PriceOffset;
    double volume_bias = config.AmountOffset;

    map<SDecimal, SDepth> result;

    for( const auto& v: depths )
    {
        SDecimal scaledPrice = v.first;

        if (config.PriceOffsetKind == 1)
        {
            if( is_ask ) 
            {
                scaledPrice *= ( 1 + price_bias );
            } 
            else 
            {
                if( price_bias < 1 )
                    scaledPrice *= ( 1 - price_bias);
                else
                    scaledPrice = 0;
            }
        }
        else if (config.PriceOffsetKind == 2)
        {
            if( is_ask ) 
            {
                scaledPrice += price_bias;
            } 
            else 
            {
                scaledPrice -= price_bias;
            }

            scaledPrice = scaledPrice > 0 ? scaledPrice : 0;
        }

        dst[v.first].mix_exchanges(v.second, 0);
        result[scaledPrice].mix_exchanges(v.second,volume_bias * (-1), config.AmountOffsetKind);
    }

    dst.swap(result);
}

void set_depth_presion(map<SDecimal, SDepth>& depth, int price_precision, int amount_precision)
{
    try
    {
        map<SDecimal, SDepth> new_depth;

        for (map<SDecimal, SDepth>::iterator iter = depth.begin(); iter!=depth.end(); ++iter)
        {
            SDecimal new_price = iter->first;
            SDepth new_atom_depth = iter->second;

            new_price.scale(price_precision);            
            new_atom_depth.volume.scale(amount_precision);
            new_depth[new_price] = new_atom_depth;
        }

        depth.swap(new_depth);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

SDepthQuote& QuoteBiasWorker::process(SDepthQuote& src, PipelineContent& ctx)
{
    try
    {
        if( ctx.params.quote_config.find(src.symbol) != ctx.params.quote_config.end() ) 
        {
            SDepthQuote tmp;
            vector<pair<SDecimal, SDepth>> depths;
            src.get_asks(depths);
            _calc_depth_bias(depths, ctx.params.quote_config[src.symbol],  true, tmp.asks);
            src.asks.swap(tmp.asks);
            src.get_bids(depths);
            _calc_depth_bias(depths, ctx.params.quote_config[src.symbol],  false, tmp.bids);
            src.bids.swap(tmp.bids);
        }
        else
        {
            LOG_WARN("QuoteConfig Can't Find " + src.symbol);
        }

        // if (src.symbol == "BTC_USDT")
        // {
        //     LOG_DEBUG("\nAfter QuoteBiasWorker: " + " " 
        //                 + src.symbol + " " + quote_str(src, 8));
        // } 
                    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return src;
}

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

void WatermarkComputerWorker::set_snap(const SDepthQuote& quote) 
{
    // 提取各交易所的买卖一

    unordered_map<TExchange, SDecimal> first_ask, first_bid;
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++ ) {
    
        // LOG_DEBUG("ask price " + iter->first.get_str_value());

        for( const auto& v : iter->second.volume_by_exchanges ) {
            const TExchange& exchange = v.first;

            //  LOG_DEBUG(exchange + " volume:" + v.second.get_str_value());

            if( first_ask.find(exchange) == first_ask.end() ) {
                first_ask[exchange] = iter->first;
            }
        }    
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {
        // LOG_DEBUG("bid price " + iter->first.get_str_value());
        for( const auto& v : iter->second.volume_by_exchanges ) {

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
        for( auto iter = watermark_.begin() ; iter != watermark_.end() ; ++iter ) {
            SymbolWatermark* obj= iter->second;
            vector<SDecimal> asks, bids;
            for( auto &v : obj->asks ) { asks.push_back(v.second); }
            for( auto &v : obj->bids ) { bids.push_back(v.second); }
            // 排序
            sort(asks.begin(), asks.end());

            // if (iter->first == "BTC_USDT")
            // {
            //     for( const auto& v : asks ) {
            //         LOG_DEBUG(iter->first + " ask " + v.get_str_value());
            //     }                
            // }


            sort(bids.begin(), bids.end());

            // if (iter->first == "BTC_USDT")
            // {
            //     for( const auto& v : bids ) {
            //         LOG_DEBUG(iter->first + " bids " + v.get_str_value());
            //     }                
            // }

            if( asks.size() > 0 && bids.size() > 0 ) {
                iter->second->watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;

                // if (iter->first == "BTC_USDT")
                // {
                //     LOG_DEBUG(iter->first +  asks[asks.size()/2].get_str_value() + " " + bids[bids.size()/2].get_str_value() 
                //                 + " " + iter->second->watermark.get_str_value());
                // }
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

        _calc_watermark();

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};
   
SDepthQuote& WatermarkComputerWorker::process(SDepthQuote& src, PipelineContent& ctx)
{

    set_snap(src);
    _calc_watermark();
    
    SDecimal watermark;
    get_watermark(src.symbol, watermark);

    // if (src.symbol == "ETH_USDT")
    // {
    //     LOG_DEBUG("\nBefore Water, water: " + watermark.get_str_value() + " Quote: \n" + quote_str(src, 5));
    // } 
     
    if (watermark.get_value() != 0 )
    {
        _filter_by_watermark(src, watermark, ctx);
    }

    // if (src.symbol == "ETH_USDT")
    // {
    //     LOG_DEBUG("\nAfter Water, water: " + watermark.get_str_value() + " Quote: " + " " 
    //                 + src.symbol + " " + quote_str(src, 5));
    // } 

    if (src.asks.size() > 0 && src.bids.size() > 0)
    {
        if (src.asks.begin()->first < src.bids.rbegin()->first)
        {
            LOG_ERROR(src.symbol + " Depth Error ask.begin: " + src.asks.begin()->first.get_str_value()
                      + " < bids.end:" +  src.bids.rbegin()->first.get_str_value());
        }
    }
    else
    {
  
    }

    // if (src.symbol == "BTC_USDT")
    // {
    //     LOG_DEBUG("\nAfter WatermarkComputerWorker: " + " " 
    //                 + src.symbol + " " + quote_str(src, 8));
    // } 
                

    return src;
}

void AccountAjdustWorker::set_snap(const SDepthQuote& quote) 
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

bool AccountAjdustWorker::get_currency(const SDepthQuote& quote, string& sell_currency, int& sell_count, string& buy_currency, int& buy_count) const
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

SDepthQuote& AccountAjdustWorker::process(SDepthQuote& src, PipelineContent& ctx)
{
    // set_snap(src);

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

    s_s << ctx.params.account_config.hedage_account_str() << "\n";
    
    unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    ctx.params.account_config.get_hedge_amounts(sell_currency, hedge_config_map, sell_total_amounts, false);
    ctx.params.account_config.get_hedge_amounts(buy_currency, hedge_config_map, buy_total_amounts, true);

    
    s_s << "\nBefore Risk sell_total_amounts " << sell_currency << "\n";
    for (auto iter:sell_total_amounts)
    {
        s_s << iter.first << ": " << iter.second << "\n";
    }

    s_s << "buy_total_amounts " << buy_currency << "\n";
    for (auto iter:buy_total_amounts)
    {
        s_s << iter.first << ": " << iter.second << "\n";
    }

    // 逐档从总余额中扣除资金消耗
    for( auto iter = src.asks.begin() ; iter != src.asks.end() ; iter++ ) 
    {
        SDepth& depth = iter->second;
        // SDecimal ori_total_volume = depth.volume;
        depth.volume = 0;
        for( auto iter2 = depth.volume_by_exchanges.begin() ; iter2 != depth.volume_by_exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second;
            double remain_amount = sell_total_amounts[exchange];

            s_s << "Ask " << exchange << " p: " + iter->first.get_str_value() + " " 
                << " need_amount: " + need_amount.get_str_value()  
                << ", remain_amount: " + std::to_string(remain_amount) << "\n";
            
            if( remain_amount < need_amount.get_value() ) 
            {
                iter2->second = 0;
                break;
            } 
            else 
            {
                sell_total_amounts[exchange] -= need_amount.get_value();
                depth.volume += iter2->second;
            }
        }
    }

    for( auto iter = src.asks.begin() ; iter != src.asks.end() ; ) 
    {
        SDepth& depth = iter->second;
        if( depth.volume.is_zero() ) {
            src.asks.erase(iter++);
        } else {
            iter++;
        }
    }
    
    for( auto iter = src.bids.rbegin() ; iter != src.bids.rend() ; iter++ ) 
    {
        SDepth& depth = iter->second;
        // SDecimal ori_total_volume = depth.volume;
        depth.volume = 0;
        
        for( auto iter2 = depth.volume_by_exchanges.begin() ; iter2 != depth.volume_by_exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second * iter->first.get_value();
            double remain_amount = buy_total_amounts[exchange];
            
            s_s << "Bid " << exchange << " p: " + iter->first.get_str_value() + " " 
                << " need_amount: " + need_amount.get_str_value()  
                << ", remain_amount: " + std::to_string(remain_amount) << "\n";

            if( remain_amount < need_amount.get_value() ) {
                iter2->second = 0;
                break;
                // LOG_DEBUG("Buy, p: " + iter->first.get_str_value() + " " + exchange + " v: " + need_amount.get_str_value() + " set to 0");
            } else {
                buy_total_amounts[exchange] -= need_amount.get_value();
                depth.volume += iter2->second;
            }
        }
    }

    for( auto iter = src.bids.begin() ; iter != src.bids.end() ; ) 
    {
        SDepth& depth = iter->second;
        if( depth.volume.is_zero() ) {
            src.bids.erase(iter++);
        } else {
            iter++;
        }
    }

    s_s << "\nAfter Risk sell_total_amounts " << sell_currency << "\n";
    for (auto iter:sell_total_amounts)
    {
        s_s << iter.first << ": " << iter.second << "\n";
    }

    s_s << "buy_total_amounts " << buy_currency << "\n";
    for (auto iter:buy_total_amounts)
    {
        s_s << iter.first << ": " << iter.second << "\n";
    }

    if (src.asks.size() == 0 || src.bids.size() == 0)
    {
        // LOG_DEBUG(s_s.str());
    }

    // if (src.symbol == "BTC_USDT")
    // {
    //     LOG_DEBUG("\nAfter AccountAjdustWorker: " + " " 
    //                 + src.symbol + " " + quote_str(src, 8));
    // } 
        
    return src;
}

SDepthQuote& OrderBookWorker::process(SDepthQuote& src, PipelineContent& ctx)
{        
    try
    {
    if (ctx.params.hedage_order_info.find(src.symbol) != ctx.params.hedage_order_info.end())
    {
        HedgeInfo& hedage_order_info = ctx.params.hedage_order_info[src.symbol];

        if (hedage_order_info.ask_amount > 0)
        {
            double ask_amount = hedage_order_info.ask_amount;

            std::vector<SDecimal> delete_price;

            for ( map<SDecimal, SDepth>::reverse_iterator iter=src.asks.rbegin(); iter != src.asks.rend(); ++iter)
            {
                if (iter->second.volume > ask_amount)
                {
                    // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                    //         + ", amount: " + iter->second.volume.get_str_value() 
                    //         + ", minus " + std::to_string(ask_amount));
                    iter->second.volume -= ask_amount;
                    break;
                }
                else
                {
                    // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                    //         + ", is deleted, delete amount: " + iter->second.volume.get_str_value() 
                    //         + ", ask_amount " + std::to_string(ask_amount));
                    ask_amount -= iter->second.volume.get_value();
                    iter->second.volume = 0;
                    delete_price.push_back(iter->first);
                }
            }

            for (auto price: delete_price)
            {
                src.asks.erase(price);
            }            
        }

        if (hedage_order_info.bid_amount > 0)
        {
            double bid_amount = hedage_order_info.bid_amount;

            std::vector<SDecimal> delete_price;

            for (auto iter: src.bids)
            {
                if (iter.second.volume > bid_amount)
                {
                    // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                    //         + ", amount: " + iter.second.volume.get_str_value() 
                    //         + ", minus " + std::to_string(bid_amount));

                    iter.second.volume -= bid_amount;
                    break;
                }
                else
                {
                    // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                    //         + ", is deleted, delete amount: " + iter.second.volume.get_str_value() 
                    //         + ", bid_amount " + std::to_string(bid_amount));

                    bid_amount -= iter.second.volume.get_value();
                    iter.second.volume = 0;
                    delete_price.push_back(iter.first);
                }
            }

            for (auto price: delete_price)
            {
                src.bids.erase(price);
            }
        }
    }

    // if (src.symbol == "BTC_USDT")
    // {
    //     LOG_DEBUG("\nAfter OrderBookWorker: " + " " 
    //                 + src.symbol + " " + quote_str(src, 8));
    // }     

    return src;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return src;
};

SDepthQuote& DefaultWorker::process(SDepthQuote& src, PipelineContent& ctx)
{
    // for( auto& v : src.asks ) {
    //     v.second.volume = 0;
    //     for( const auto& v2 : v.second.volume_by_exchanges ) {
    //         v.second.volume += v2.second;
    //     }
    // }
    // for( auto& v : src.bids ) {
    //     v.second.volume = 0;
    //     for( const auto& v2 : v.second.volume_by_exchanges ) {
    //         v.second.volume += v2.second;
    //     }
    // }

    return src;
}

SDepthQuote& PrecisionWorker::process(SDepthQuote& src, PipelineContent& ctx)
{
    try
    {
        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
        {
            int price_precision = ctx.params.symbol_config[src.symbol].PricePrecision;
            int amount_precision = ctx.params.symbol_config[src.symbol].AmountPrecision;

            set_depth_presion(src.asks, price_precision, amount_precision);
            set_depth_presion(src.bids, price_precision, amount_precision);

            if (src.symbol == "BTC_USDT")
            {
                LOG_DEBUG("price_precision: " + std::to_string(price_precision) 
                        + ", amount_precision: " + std::to_string(amount_precision)
                        + "\n" + quote_str(src, 5));
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return src;
}

QuotePipeline::QuotePipeline(){
    add_worker(&default_worker_);
}

QuotePipeline::~QuotePipeline(){

}