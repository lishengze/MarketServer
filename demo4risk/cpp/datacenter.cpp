#include "risk_controller_config.h"
#include "datacenter.h"
#include "grpc_server.h"
#include "converter.h"
#include "updater_configuration.h"

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

        for( auto v = src_depths.begin() ; v != src_depths.end() ; )
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;

            // cout << price.get_str_value() << " " << watermark.get_str_value() << endl;
            if( is_ask ? (price <= watermark) : (price >= watermark) ) {
                // 过滤价位
                for( const auto& v2 : depth.exchanges ){
                    volumes[v2.first] += v2.second;
                }
                src_depths.erase(v++);
                // if (src.symbol == "BTC_USDT")
                // {
                //     std::cout << "filed_price: " << price.get_str_value() << " watermark: " << watermark.get_str_value() << endl;
                // }
                cout << "-- delete ask depth " << src.symbol << ", price: " << price.get_str_value() << ", water: " << watermark.get_value() << endl;
            } else {
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
                        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
                        {
                            SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];

                            SDecimal new_price = watermark + symbol_config.MinChangePrice;

                            src_depths[new_price] = fake;

                            cout << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].total_volume.get_value() << ", "
                                << "is_ask: " << is_ask
                                << endl;                        
                        }
                        else
                        {
                            cout << "No New Price " << src.symbol << ", "
                                << "volume: " << fake.total_volume.get_value() << ", "
                                << "is_ask: " << is_ask
                                << endl;                             
                            depth.mix_exchanges(fake, 0, 1);
                        }
                    }
                    
                    // 
                } else {
                    break;
                }
                // cout << "accumulate to depth " << price.get_str_value() << endl;
                v++;
            }
        }
    }
    else 
    {
        map<SDecimal, SInnerDepth>& src_depths = src.bids;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一

        for( auto v = src_depths.rbegin() ; v != src_depths.rend() ; )
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;
            // cout << price.get_str_value() << " " << watermark.get_str_value() << endl;
            if( is_ask ? (price <= watermark) : (price >= watermark) ) {
                // 过滤价位
                for( const auto& v2 : depth.exchanges ){
                    volumes[v2.first] += v2.second;
                }
                v = decltype(v)(src_depths.erase( std::next(v).base() ));
                cout << "-- delete bid depth " << src.symbol << ", price: " << price.get_str_value() << ", water: " << watermark.get_value() << endl;
            } else {
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
                        if (ctx.params.symbol_config.find(src.symbol) != ctx.params.symbol_config.end())
                        {
                            SymbolConfiguration& symbol_config = ctx.params.symbol_config[src.symbol];

                            SDecimal new_price = watermark - symbol_config.MinChangePrice;

                            src_depths[new_price] = fake;

                            cout << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].total_volume.get_value() << ", "
                                << "is_ask: " << is_ask
                                << endl;

                        }
                        else
                        {
                            cout << "No New Price " << src.symbol << ", "
                                << "volume: " << fake.total_volume.get_value() << ", "
                                << "is_ask: " << is_ask
                                << endl;                        
                            depth.mix_exchanges(fake, 0, 1);
                        }
                    }



                } else {
                    break;
                }
                // cout << "accumulate to depth " << price.get_str_value() << endl;
                v++;
            }
        }
    }
}

void _filter_by_watermark(SInnerQuote& src, const SDecimal& watermark, PipelineContent& ctx)
{
    _filter_depth_by_watermark(src, watermark, true, ctx);
    _filter_depth_by_watermark(src, watermark, false, ctx);
}

void _calc_depth_bias(const vector<pair<SDecimal, SInnerDepth>>& depths, QuoteConfiguration& config, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    double price_bias = config.PriceOffset;
    double volume_bias = config.AmountOffset;

    // if (config.symbol == "BTC_USDT")
    // {
    //     std::cout << config.symbol << " " << config.PriceOffsetKind << " " << config.PriceOffset << " "
    //             << config.AmountOffsetKind << " " << config.AmountOffset << std::endl;
    // }

    map<SDecimal, SInnerDepth> result;

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

        // if (config.symbol == "BTC_USDT")
        // {
        //     std::cout << config.symbol << " " << is_ask 
        //               << " ori_price: " << v.first.get_str_value() 
        //               << " offset_price: " << scaledPrice.get_str_value()
        //               << " ori_volume: " << dst[v.first].total_volume.get_str_value() 
        //               << " offset_volume: " << result[scaledPrice].total_volume.get_str_value()
        //               << std::endl;
        // }


        // dst[scaledPrice].mix_exchanges(v.second, volume_bias * (-1), config.AmountOffsetKind);
    }

    dst.swap(result);
}

SInnerQuote& QuoteBiasWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    try
    {
        if( ctx.params.cache_config.find(src.symbol) != ctx.params.cache_config.end() ) 
        {
            SInnerQuote tmp;
            vector<pair<SDecimal, SInnerDepth>> depths;
            src.get_asks(depths);
            _calc_depth_bias(depths, ctx.params.cache_config[src.symbol],  true, tmp.asks);
            src.asks.swap(tmp.asks);
            src.get_bids(depths);
            _calc_depth_bias(depths, ctx.params.cache_config[src.symbol],  false, tmp.bids);
            src.bids.swap(tmp.bids);
        }
        else
        {
            /* code */
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    // tfm::printfln("QuoteBiasWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());

    return src;
}

WatermarkComputerWorker::WatermarkComputerWorker() 
{
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
        // if (quote.symbol == "BTC_USDT")
        // {
        //     std::cout << "ask price " << iter->first.get_str_value() << " ";
        // }

        for( const auto& v : iter->second.exchanges ) {
            const TExchange& exchange = v.first;

            // if (quote.symbol == "BTC_USDT")
            // {
            //     std::cout << exchange << " volume: " << v.second.get_str_value() << " ";
            // }

            
            if( first_ask.find(exchange) == first_ask.end() ) {
                first_ask[exchange] = iter->first;
            }
        }

        // if (quote.symbol == "BTC_USDT")
        // {
        //     std::cout << std::endl;
        // }        
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {
        // if (quote.symbol == "BTC_USDT")
        // {
        //     std::cout << "bid price " << iter->first.get_str_value() << " ";
        // }

        for( const auto& v : iter->second.exchanges ) {

            // if (quote.symbol == "BTC_USDT")
            // {
            //     std::cout << v.first << " volume: " << v.second.get_str_value() << " ";
            // }

            const TExchange& exchange = v.first;
            if( first_bid.find(exchange) == first_bid.end() ) {
                first_bid[exchange] = iter->first;
            }
        }

        // if (quote.symbol == "BTC_USDT")
        // {
        //     std::cout << std::endl;
        // }           
    }

    // if (quote.symbol == "BTC_USDT")
    // {
    //     for (auto iter:first_ask)
    //     {
    //         std::cout << "first_ask " << iter.first << " " << iter.second.get_str_value() << std::endl;
    //     }

    //     for (auto iter:first_bid)
    //     {
    //         std::cout << "first_bid " << iter.first << " " << iter.second.get_str_value() << std::endl;
    //     }        
    // }    



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
        for( auto iter = watermark_.begin() ; iter != watermark_.end() ; ++iter ) {
            SymbolWatermark* obj= iter->second;
            vector<SDecimal> asks, bids;
            for( auto &v : obj->asks ) { asks.push_back(v.second); }
            for( auto &v : obj->bids ) { bids.push_back(v.second); }
            // 排序
            sort(asks.begin(), asks.end());

            // if (iter->first == "BTC_USDT")
            // {
            // for( const auto& v : asks ) {
            //     cout << "ask " << v.get_str_value() << endl;
            // }                
            // }


            sort(bids.begin(), bids.end());
            //for( const auto& v : bids ) {
                // cout << "bid " << v.get_str_value() << endl;
            //}
            if( asks.size() > 0 && bids.size() > 0 ) {
                iter->second->watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;

                // if (iter->first == "BTC_USDT")
                // {
                //     cout << asks[asks.size()/2].get_str_value() << " " << bids[bids.size()/2].get_str_value() << " " << iter->second->watermark.get_str_value() << endl;
                // }
            }
        }
    }
    catch(const std::exception& e)
    {
        _log_and_print("Exception: %s", e.what());

        // std::cerr << "" << e.what() << '\n';
    }        
}

void WatermarkComputerWorker::thread_func() {

    while( thread_run_ ) {

        _calc_watermark();

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};
   
SInnerQuote& WatermarkComputerWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    if (src.symbol == "BTC_USDT")
    {
        // std::cout << "WatermarkComputerWorker::process " << src.symbol << ", " << src.asks.size() << "/" <<  src.bids.size()<< std::endl;        
    }
    set_snap(src);
    
    SDecimal watermark;
    get_watermark(src.symbol, watermark);
    _calc_watermark();
    if (watermark.get_value() != 0 )
    {
        _filter_by_watermark(src, watermark, ctx);
    }
    
    // _log_and_print("worker(watermark)-%s: %s %lu/%lu", src.symbol.c_str(), watermark.get_str_value().c_str(), src.asks.size(), src.bids.size());
    if (src.symbol == "BTC_USDT")
    {
        // tfm::printfln("WatermarkComputerWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());
    }

    return src;
}

void AccountAjdustWorker::set_snap(const SInnerQuote& quote) 
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
    // if (src.symbol == "BTC_USDT")
    // {
    //     std::cout << "AccountAjdustWorker::process " << src.symbol << ", " << src.asks.size() << ", " <<  src.bids.size()<< std::endl;
    // }

    // 获取配置
    double hedge_percent = 0;
    auto iter = ctx.params.cache_config.find(src.symbol);
    if( iter != ctx.params.cache_config.end() ) {
        hedge_percent = iter->second.HedgeFundRatio;
    }

    // 获取币种出现次数
    string sell_currency, buy_currency;
    int sell_count, buy_count;
    if( !get_currency(src, sell_currency, sell_count, buy_currency, buy_count) )
        return src;

    // 动态调整每个品种的资金分配量
    unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    ctx.params.cache_account.get_hedge_amounts(sell_currency, hedge_percent / sell_count, sell_total_amounts);
    ctx.params.cache_account.get_hedge_amounts(buy_currency, hedge_percent / buy_count, buy_total_amounts);

    // 逐档从总余额中扣除资金消耗
    for( auto iter = src.asks.begin() ; iter != src.asks.end() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        depth.total_volume = 0;
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second;
            double remain_amount = sell_total_amounts[exchange];
            if( remain_amount < need_amount.get_value() ) {
                iter2->second = 0;
            } else {
                sell_total_amounts[exchange] -= need_amount.get_value();
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
    
    for( auto iter = src.bids.rbegin() ; iter != src.bids.rend() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        depth.total_volume = 0;
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second * iter->first.get_value();
            double remain_amount = sell_total_amounts[exchange];
            if( remain_amount < need_amount.get_value() ) {
                iter2->second = 0;
                //cout << remain_amount << " " << iter2->second.get_str_value() << " " << iter->first.get_str_value() << " " << need_amount.get_str_value() << endl;
            } else {
                sell_total_amounts[exchange] -= need_amount.get_value();
                depth.total_volume += iter2->second;
                //cout << remain_amount << " " << iter2->second.get_str_value() << " " << iter->first.get_str_value() << " " << depth.total_volume.get_str_value() << endl;
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

    // if (src.symbol == "BTC_USDT")
    // {
    //     tfm::printfln("AccountAjdustWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());
    // }

    
    return src;
}

SInnerQuote& OrderBookWorker::process(SInnerQuote& src, PipelineContent& ctx)
{        
    if (ctx.params.hedage_info.find(src.symbol) != ctx.params.hedage_info.end())
    {
        HedgeInfo& hedage_info = ctx.params.hedage_info[src.symbol];

        if (hedage_info.ask_amount > 0)
        {
            double ask_amount = hedage_info.ask_amount;

            std::vector<SDecimal> delete_price;

            for ( map<SDecimal, SInnerDepth>::reverse_iterator iter=src.asks.rbegin(); iter != src.asks.rend(); ++iter)
            {
                if (iter->second.total_volume > ask_amount)
                {
                    iter->second.total_volume -= ask_amount;
                    break;
                }
                else
                {
                    iter->second.total_volume = 0;
                    ask_amount -= iter->second.total_volume.get_value();
                    delete_price.push_back(iter->first);
                }
            }

            for (auto price: delete_price)
            {
                src.asks.erase(price);
            }            
        }

        if (hedage_info.bid_amount > 0)
        {
            double bid_amount = hedage_info.bid_amount;

            std::vector<SDecimal> delete_price;

            for (auto iter: src.bids)
            {
                if (iter.second.total_volume > bid_amount)
                {
                    iter.second.total_volume -= bid_amount;
                    break;
                }
                else
                {
                    iter.second.total_volume = 0;
                    bid_amount -= iter.second.total_volume.get_value();
                    delete_price.push_back(iter.first);
                }
            }

            for (auto price: delete_price)
            {
                src.bids.erase(price);
            }
        }

    }
    return src;
    /*
    auto orderBookIter = ctx.params.cache_order.find(src->symbol);
    if( orderBookIter != ctx.params.cache_order.end() ) 
    {
        // 卖盘
        for( uint32 i = 0 ; i < src->ask_length ; ) {
            const vector<SOrderPriceLevel>& levels = orderBookIter->second.first;
            for( auto iter = levels.begin() ; iter != levels.end() ; ) {
                if( iter->price < src->asks[i].price ) {
                    iter ++;
                } else if( iter->price > src->asks[i].price ) {
                    i++;
                } else {
                    src->asks[i].total_volume -= iter->volume;
                }
            }
        }
        // 买盘
        for( uint32 i = 0 ; i < src->bid_length ; ) {
            const vector<SOrderPriceLevel>& levels = orderBookIter->second.second;
            for( auto iter = levels.begin() ; iter != levels.end() ; ) {
                if( iter->price < src->bids[i].price ) {
                    iter ++;
                } else if( iter->price > src->bids[i].price ) {
                    i++;
                } else {
                    src->bids[i].total_volume -= iter->volume;
                }
            }
        }
    }*/
    return src;
};

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
    return src;
}

QuotePipeline::QuotePipeline(){
    add_worker(&default_worker_);
}

QuotePipeline::~QuotePipeline(){

}
/////////////////////////////////////////////////////////////////////////////////
DataCenter::DataCenter() {
    pipeline_.add_worker(&quotebias_worker_);
    pipeline_.add_worker(&watermark_worker_);
    pipeline_.add_worker(&account_worker_);
    //pipeline_.add_worker(&orderbook_worker_);
}

DataCenter::~DataCenter() {

}

void DataCenter::add_quote(const SInnerQuote& quote)
{    
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    if (strcmp(quote.exchange.c_str(), MIX_EXCHANGE_NAME) == 0 && quote.symbol == "BTC_USDT")
    {
        _log_and_print("Receive Raw Data %s.%s %u/%u", quote.exchange, quote.symbol, quote.asks.size(), quote.bids.size());
        // for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++ ) 
        // {

        //         std::cout << "ask price " << iter->first.get_str_value() << " ";

        //     for( const auto& v : iter->second.exchanges ) {
        //         const TExchange& exchange = v.first;

        //         std::cout << exchange << " volume: " << v.second.get_str_value() << " ";

        //     }
        //     std::cout << std::endl;     
        // }
        // for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {

        //     std::cout << "bid price " << iter->first.get_str_value() << " ";
            
        //     for( const auto& v : iter->second.exchanges ) {
        //         std::cout << v.first << " volume: " << v.second.get_str_value() << " ";                
        //     }

        //     std::cout << std::endl;       
        // }
    }



    innerquote_to_msd2(quote, ptrData.get(), false);
    //std::cout << "publish for broker " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;
    for( const auto& v : callbacks_) 
    {
        v->publish4Hedge(quote.symbol, ptrData, NULL);
    }

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    // 存储原始行情
    datas_[quote.symbol] = quote;    
    // 发布行情
    _publish_quote(quote);
};

void DataCenter::change_account(const AccountInfo& info)
{
    tfm::printfln("change_account");
    
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_account = info;
    _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, QuoteConfiguration>& config)
{
    tfm::printfln("change_configuration");
    
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_config = config;

    for (auto iter:config)
    {
        cout << iter.second.desc() << endl;
    }

    std::cout << "\nDataCenter::change_configuration  " << std::endl;

    // for (auto iter:params_.cache_config)
    // {
    //     std::cout << iter.first << "  PriceOffset: " << iter.second.PriceOffset  << ", AmountOffset: " << iter.second.AmountOffset << std::endl;
    // }    
    _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        cout << "DataCenter::change_configuration SymbolConfiguration" << endl;

        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.symbol_config = config;

        for (auto iter:params_.symbol_config)
        {
            cout << iter.second.desc() << endl;
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void DataCenter::change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
    tfm::printfln("change_orders");

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_order[symbol] = make_pair(asks, bids);

    _push_to_clients(symbol);
}

void DataCenter::_push_to_clients(const TSymbol& symbol) 
{
    if( symbol == "" ) 
    {
        for( auto iter = datas_.begin() ; iter != datas_.end() ; ++iter ) 
        {
            _publish_quote(iter->second);
        }
    } else {
        auto iter = datas_.find(symbol);
        if( iter == datas_.end() )
            return;

        _publish_quote(iter->second);
    }
}

void DataCenter::_publish_quote(const SInnerQuote& quote) 
{    
    SInnerQuote newQuote;

    // std::cout << "\nparams_.cache_config  " << std::endl;

    // for (auto iter:params_.cache_config)
    // {
    //     std::cout << iter.first << "  PriceOffset: " << iter.second.PriceOffset  << ", AmountOffset: " << iter.second.AmountOffset << std::endl;
    // }

    pipeline_.run(quote, params_, newQuote);

    // std::cout << "\nAfter Pipeline:  " << newQuote.symbol << " " << newQuote.asks.size() << " / "<< newQuote.bids.size() << std::endl;

    // 检查是否发生变化
    auto iter = last_datas_.find(quote.symbol);
    if( iter != last_datas_.end() ) {
        const SInnerQuote& last_quote = iter->second;
        if( quote.time_origin <= last_quote.time_origin ) {
            //tfm::printfln("%s %ul %ul", quote.symbol, quote.time_origin, last_quote.time_origin);
            return;
        }
    }


    // if (!(params_.cache_config.find(quote.symbol) != params_.cache_config.end()
    // && params_.cache_config[quote.symbol].IsPublish))
    // {
    //     // _log_and_print(" %s UnPublished", quote.symbol);
    //     // std::cout << quote.symbol << " UnPublished!" << std::endl;
    //     return;
    // }

    last_datas_[quote.symbol] = newQuote;

    if (!check_quote(newQuote))
    {
        return;
    }

    // std::cout << "publish(raw) " << quote.symbol << " " << newQuote.asks.size() << "/"<< newQuote.bids.size() << std::endl;
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd2(newQuote, ptrData.get(), true);    
    //std::cout << "publish " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;
    // _log_and_print("Publish4Broker %s.%s %u/%u", quote.exchange, quote.symbol, ptrData->asks_size(), ptrData->bids_size());
    for( const auto& v : callbacks_) 
    {
        v->publish4Broker(quote.symbol, ptrData, NULL);
    }

    // send to clients
    std::shared_ptr<MarketStreamDataWithDecimal> ptrData2(new MarketStreamDataWithDecimal);
    innerquote_to_msd3(newQuote, ptrData2.get(), true);   

    // if (strcmp(quote.exchange.c_str(), MIX_EXCHANGE_NAME) == 0)
    // {
    //     // _log_and_print("Publish4Client %s.%s %u/%u", quote.exchange, quote.symbol, ptrData2->asks_size(), ptrData2->bids_size());
    // }
    
    for( const auto& v : callbacks_) 
    {
        v->publish4Client(quote.symbol, ptrData2, NULL);
    }

    
}

bool DataCenter::check_quote(SInnerQuote& quote)
{
    bool result = false;
    if (params_.cache_config.find(quote.symbol) != params_.cache_config.end())
    {
        result = params_.cache_config[quote.symbol].IsPublish;

        if (!result) return result;

        uint32 publis_level = params_.cache_config[quote.symbol].PublishLevel;
        map<SDecimal, SInnerDepth> new_asks;
        map<SDecimal, SInnerDepth> new_bids;     

        if (strcmp(quote.exchange.c_str(), MIX_EXCHANGE_NAME) == 0 && quote.symbol == "BTC_USDT")
        {
            std::cout << quote.symbol << " old_size: " << quote.asks.size() << " / " << quote.bids.size() << " ";
        }

        // std::cout << quote.symbol << " old_size: " << quote.asks.size() << " / " << quote.bids.size() << " ";

        int i = 0;
        for (auto iter = quote.asks.begin(); iter != quote.asks.end() && i < publis_level; ++iter, ++i)
        {
            new_asks[iter->first] = iter->second;
        }   
        quote.asks.swap(new_asks);

        i = 0;
        for(auto iter = quote.bids.rbegin(); iter != quote.bids.rend() && i < publis_level; ++iter, ++i)
        {
            new_bids[iter->first] = iter->second;
        }
        quote.bids.swap(new_bids);

        if (strcmp(quote.exchange.c_str(), MIX_EXCHANGE_NAME) == 0 && quote.symbol == "BTC_USDT")
        {
            std::cout << " new_size: " << quote.asks.size() << " / " << quote.bids.size() << endl;
        }        

        // std::cout << " new_size: " << quote.asks.size() << " / " << quote.bids.size() << endl;
    }

    return result;
    
}

void reset_price(double& price, QuoteConfiguration& config, bool is_ask)
{
    try
    {
        double price_bias = config.PriceOffset;

        if (config.PriceOffsetKind == 1)
        {
            if( is_ask ) 
            {
                price /= ( 1 + price_bias );
            } 
            else 
            {
                if( price_bias < 1 )
                    price /= ( 1 - price_bias);
                else
                    price = 0;
            }
        }
        else if (config.PriceOffsetKind == 2)
        {
            if( is_ask ) 
            {
                price -= price_bias;
            } 
            else 
            {
                price += price_bias;
            }

            price = price > 0 ? price : 0;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

QuoteResponse_Result _calc_otc_by_amount(const map<SDecimal, SInnerDepth>& depths, bool is_ask, QuoteConfiguration& config, double volume, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0, total_amount;
    if( is_ask ) {

        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, true);
            // cout << "ori_price: " << iter->first.get_value() << ", trans_price: " << price << " " << config.PriceOffsetKind << " " << config.PriceOffset << endl;

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * price;
            } else {
                total_amount += (volume - total_volume.get_value()) * price;
                total_volume = volume;
            }

            cout << "total_volume: " << total_volume.get_value() << endl;
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, false);

            // cout << "ori_price: " << iter->first.get_value() << ", trans_price: " << price << " " << config.PriceOffsetKind << " " << config.PriceOffset << endl;

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * price;
            } else {
                total_amount += (volume - total_volume.get_value()) * price;
                total_volume = volume;
            }

            cout << "total_volume: " << total_volume.get_value() << endl;
        }
    }

    if( total_volume < volume )
        return QuoteResponse_Result_NOT_ENOUGH_VOLUME;

    price = total_amount.get_value() / total_volume.get_value();
    std::cout << "ori_price: " << price.get_value() << " ";
    if (config.OTCOffsetKind == 1)
    {
        if( is_ask ) {
            price *= ( 1 + config.OtcOffset); 
        } else {
            price *= ( 1 - config.OtcOffset); 
        }
    }
    else if (config.OTCOffsetKind == 2)
    {
        if( is_ask ) {
            price += config.OtcOffset;
        } else {
            price -= config.OtcOffset;
        }        
    }
    std::cout << "bias_price: " << price.get_value() 
              << " bias_kind: " << config.OTCOffsetKind 
              << " bias_value: " << config.OtcOffset 
              << std::endl;
    price.scale(precise, is_ask);
    return QuoteResponse_Result_OK;
}

QuoteResponse_Result _calc_otc_by_turnover(const map<SDecimal, SInnerDepth>& depths, bool is_ask, QuoteConfiguration& config, double amount, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0, total_amount;
    if( is_ask ) {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, true);
            // cout << "ori_price: " << iter->first.get_value() << ", trans_price: " << price << " " << config.PriceOffsetKind << " " << config.PriceOffset<< endl;

            SDecimal amounts = iter->second.total_volume * price;
            if( (total_amount + amounts) <= amount ) {
                total_volume += iter->second.total_volume;
                total_amount += amounts;
            } else {
                total_volume += (amount - total_amount.get_value()) / price;
                total_amount = amount;
                break;
            }
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, false);
            // cout << "ori_price: " << iter->first.get_value() << ", trans_price: " << price << " " << config.PriceOffsetKind << " " << config.PriceOffset<< endl;
            SDecimal amounts = iter->second.total_volume * price;
            if( (total_amount + amounts) <= amount ) {
                total_volume += iter->second.total_volume;
                total_amount += amounts;
            } else {
                total_volume += (amount - total_amount.get_value()) / price;
                total_amount = amount;
                break;
            }
        }
    }
    if( total_amount < amount )
        return QuoteResponse_Result_NOT_ENOUGH_AMOUNT;

    price = total_amount.get_value() / total_volume.get_value();

    std::cout << "ori_price: " << price.get_value() << " ";
    if (config.OTCOffsetKind == 1)
    {
        if( is_ask ) {
            price *= ( 1 + config.OtcOffset); 
        } else {
            price *= ( 1 - config.OtcOffset); 
        }
    }
    else if (config.OTCOffsetKind == 2)
    {
        if( is_ask ) {
            price += config.OtcOffset;
        } else {
            price -= config.OtcOffset;
        }        
    }

    std::cout << "bias_price: " << price.get_value() 
              << " bias_kind: " << config.OTCOffsetKind 
              << " bias_value: " << config.OtcOffset 
              << std::endl;

    price.scale(precise, is_ask);
    return QuoteResponse_Result_OK;
}

bool DataCenter::get_snaps(vector<SInnerQuote>& snaps)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( const auto& v: last_datas_ ) {
        snaps.push_back(v.second);
    }
    return true;
}

QuoteResponse_Result DataCenter::otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double amount, double turnover, SDecimal& price)
{
    _log_and_print("[otc_query] %s.%s direction=%s amount=%s turnover=%s", exchange, symbol, direction, amount, turnover);
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    auto iter = last_datas_.find(symbol);
    for (auto iter:last_datas_)
    {
        cout << iter.first << endl;
    }
    cout << "last_datas_.size: " << last_datas_.size() << endl;
    if( iter == last_datas_.end() )
        return QuoteResponse_Result_WRONG_SYMBOL;

    SInnerQuote& quote = iter->second;

    std::cout << "\n*****Config Info: " << symbol << endl;
    std::cout << params_.cache_config[symbol].desc() << std::endl;

    if( amount > 0 )
    {
        cout << "Compute "<< symbol << " Amount " <<  amount << endl;
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_amount(quote.asks, true, params_.cache_config[symbol], amount, price, quote.precise);
        } else {
            return _calc_otc_by_amount(quote.bids, false, params_.cache_config[symbol], amount, price, quote.precise);   
        }
    } 
    else
    {
        cout << "Compute "<< symbol << " turnover" <<  turnover << endl;
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_turnover(quote.asks, true, params_.cache_config[symbol], turnover, price, quote.precise);
        } else { 
            return _calc_otc_by_turnover(quote.bids, false, params_.cache_config[symbol], turnover, price, quote.precise);
        }
    }

    return QuoteResponse_Result_WRONG_DIRECTION;
}

void DataCenter::get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations)
{
    watermark_worker_.query(watermarks);

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( const auto&v : params_.cache_account.hedge_accounts_ ) {
        const TExchange& exchange = v.first;
        for( const auto& v2 : v.second.currencies ) {
            const TSymbol& symbol = v2.first;
            accounts[exchange][symbol] = v2.second.amount;
        }
    }
    for( const auto&v : params_.cache_config ) {
        const TSymbol& symbol = v.first;
        configurations[symbol] = v.second.desc();
    }
}

void DataCenter::hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade)
{
    try
    {
        if (params_.hedage_info.find(symbol) == params_.hedage_info.end() && !is_trade)
        {
            params_.hedage_info[symbol] = HedgeInfo(symbol, price, amount, direction, is_trade);
        }
        else
        {
            params_.hedage_info[symbol].set(symbol, price, amount, direction, is_trade);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}
