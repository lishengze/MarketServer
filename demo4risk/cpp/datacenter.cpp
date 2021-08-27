#include "risk_controller_config.h"
#include "datacenter.h"
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

        for( auto v = src_depths.begin() ; v != src_depths.end() ; )
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;

            if( is_ask ? (price <= watermark) : (price >= watermark) ) {
                // 过滤价位
                for( const auto& v2 : depth.exchanges ){
                    volumes[v2.first] += v2.second;
                }
                src_depths.erase(v++);
                // LOG_DEBUG("watermark filter ask " + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
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

                            stringstream s_s;
                            s_s << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].total_volume.get_value() << ", "
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
    else 
    {
        map<SDecimal, SInnerDepth>& src_depths = src.bids;
        bool patched = false;
        unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一

        for( auto v = src_depths.rbegin() ; v != src_depths.rend() ; )
        {
            const SDecimal& price = v->first;
            SInnerDepth& depth = v->second;
            if( is_ask ? (price <= watermark) : (price >= watermark) ) {
                // 过滤价位
                for( const auto& v2 : depth.exchanges ){
                    volumes[v2.first] += v2.second;
                }
                v = decltype(v)(src_depths.erase( std::next(v).base() ));
                // LOG_DEBUG("watermark filter bid" + src.symbol + ", depth_price: " + price.get_str_value() + ", water: " + watermark.get_str_value());
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

                            stringstream s_s;
                            s_s << "Add New Price: " << src.symbol << ", "
                                << "new price: " << new_price.get_value() << ", "
                                << "volume: " << src_depths[new_price].total_volume.get_value() << ", "
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

void _filter_by_watermark(SInnerQuote& src, const SDecimal& watermark, PipelineContent& ctx)
{
    _filter_depth_by_watermark(src, watermark, true, ctx);
    _filter_depth_by_watermark(src, watermark, false, ctx);
}

void _calc_depth_bias(const vector<pair<SDecimal, SInnerDepth>>& depths, QuoteConfiguration& config, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    double price_bias = config.PriceOffset;
    double volume_bias = config.AmountOffset;

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
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
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

void WatermarkComputerWorker::set_snap(const SInnerQuote& quote) 
{
    // 提取各交易所的买卖一
    // LOG_DEBUG("WatermarkComputerWorker::set_snap");

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
   
SInnerQuote& WatermarkComputerWorker::process(SInnerQuote& src, PipelineContent& ctx)
{

    set_snap(src);
    _calc_watermark();
    
    SDecimal watermark;
    get_watermark(src.symbol, watermark);
    
    if (watermark.get_value() != 0 )
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

    if (filter_zero_volume(src))
    {
        LOG_WARN("\n" + src.symbol + " After WatermarkComputerWorker \n" + quote_str(src));
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
            } else {
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

    // if (filter_zero_volume(src))
    // {
    //     LOG_DEBUG("\n" + src.symbol + " After AccountAjdustWorker \n" + quote_str(src));
    //     LOG_WARN( "After AccountAjdustWorker" + src.symbol + ", ask.size: " + std::to_string(src.asks.size())
    //             + ", bid.size: " + std::to_string(src.bids.size())) ;            
    // }

    return src;
}

SInnerQuote& OrderBookWorker::process(SInnerQuote& src, PipelineContent& ctx)
{        
    try
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
                    // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                    //         + ", amount: " + iter->second.total_volume.get_str_value() 
                    //         + ", minus " + std::to_string(ask_amount));
                    iter->second.total_volume -= ask_amount;
                    break;
                }
                else
                {
                    // LOG_DEBUG(src.symbol + " ask_price: " + iter->first.get_str_value() 
                    //         + ", is deleted, delete amount: " + iter->second.total_volume.get_str_value() 
                    //         + ", ask_amount " + std::to_string(ask_amount));
                    ask_amount -= iter->second.total_volume.get_value();
                    iter->second.total_volume = 0;
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
                    // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                    //         + ", amount: " + iter.second.total_volume.get_str_value() 
                    //         + ", minus " + std::to_string(bid_amount));

                    iter.second.total_volume -= bid_amount;
                    break;
                }
                else
                {
                    // LOG_DEBUG(src.symbol + " bid_price: " + iter.first.get_str_value() 
                    //         + ", is deleted, delete amount: " + iter.second.total_volume.get_str_value() 
                    //         + ", bid_amount " + std::to_string(bid_amount));

                    bid_amount -= iter.second.total_volume.get_value();
                    iter.second.total_volume = 0;
                    delete_price.push_back(iter.first);
                }
            }

            for (auto price: delete_price)
            {
                src.bids.erase(price);
            }
        }

    }

    // if (filter_zero_volume(src))
    // {
    //     LOG_DEBUG("\n" + src.symbol + " After OrderBookWorker\n" + quote_str(src));
    //     LOG_WARN( "After OrderBookWorker " + src.symbol + ", ask.size: " + std::to_string(src.asks.size())
    //             + ", bid.size: " + std::to_string(src.bids.size())) ;             
    // }    

    return src;
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
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
    pipeline_.add_worker(&orderbook_worker_);
}

DataCenter::~DataCenter() {

}

void DataCenter::add_quote(const SInnerQuote& quote)
{    
    if (filter_zero_volume( const_cast<SInnerQuote&>(quote)))
    {
        LOG_DEBUG("\n" + quote.symbol + " raw quote\n" + quote_str(quote));     
        LOG_WARN("ori_data" + quote.symbol + ", ask.size: " + std::to_string(quote.asks.size())
                + ", bid.size: " + std::to_string(quote.bids.size())) ;
        return;
    }    

    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);

    innerquote_to_msd2(quote, ptrData.get(), false);

    LOG->record_output_info("Hedge_" + quote.symbol + "_" + quote.exchange);
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
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_account = info;
    _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, QuoteConfiguration>& config)
{   
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_config = config;

    LOG_INFO("DataCenter::change QuoteConfiguration");
    for (auto iter:params_.cache_config)
    {
        LOG_INFO("\n" + iter.first + "\n" + iter.second.desc());
    }    
    _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.symbol_config = config;

        LOG_INFO("DataCenter::change SymbolConfiguration");
        for (auto iter:params_.symbol_config)
        {
            LOG_INFO("\n" + iter.first + " " + iter.second.desc());
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
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

    pipeline_.run(quote, params_, newQuote);

    // 检查是否发生变化
    auto iter = last_datas_.find(quote.symbol);
    if( iter != last_datas_.end() ) {
        const SInnerQuote& last_quote = iter->second;
        if( quote.time_origin < last_quote.time_origin ) {
            LOG_WARN("Quote Error last_quote.time: " + std::to_string(last_quote.time_origin) + ", processed quote_time: " + std::to_string(quote.time_origin));
            return;
        }
    }

    last_datas_[quote.symbol] = newQuote;

    if (!check_quote(newQuote))
    {
        LOG_WARN(quote.symbol + " not published!" );
        return;
    }

     if (filter_zero_volume(newQuote))
    {
        LOG_WARN("\n" + quote.symbol + " _publish_quote \n" + quote_str(quote));
        return;  
    }    

    
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd2(newQuote, ptrData.get(), true);    

    LOG->record_output_info("Broker_" + quote.symbol + "_" + quote.exchange);

    for( const auto& v : callbacks_) 
    {
        v->publish4Broker(quote.symbol, ptrData, NULL);
    }

    // send to clients
    std::shared_ptr<MarketStreamDataWithDecimal> ptrData2(new MarketStreamDataWithDecimal);
    innerquote_to_msd3(newQuote, ptrData2.get(), true);   

    LOG->record_output_info("Client_" + quote.symbol + "_" + quote.exchange);

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
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
}

QuoteResponse_Result _calc_otc_by_volume(const map<SDecimal, SInnerDepth>& depths, bool is_ask, QuoteConfiguration& config, double volume, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0; 
    SDecimal total_amount = 0;

    if( is_ask ) {

        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            
            double price = iter->first.get_value();
            double old_price = price;
            reset_price(price, config, true);

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * price;

                LOG_DEBUG("cur_p: " + std::to_string(price)
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", ask;");

            } else {
                total_amount += (volume - total_volume.get_value()) * price;
                total_volume = volume;

                LOG_DEBUG("done cur_p: " +  std::to_string(price) 
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", ask;");
                break;
            }
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            double old_price = price;
            reset_price(price, config, false);

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                double trade_amout = iter->second.total_volume.get_value() * price;
                total_amount += trade_amout;

                LOG_DEBUG("cur_p: " + std::to_string(price)
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", cur_a: " + std::to_string(trade_amout)
                        + ", total_a: " + total_amount.get_str_value()
                        + ", otc_v: " + std::to_string(volume)                        
                        + ", bid;");

            } else {
                double trade_volume = volume - total_volume.get_value();
                double trade_amout = trade_volume * price;
                total_amount += trade_amout;
                total_volume += trade_volume;

                LOG_DEBUG("[done] cur_price: " + std::to_string(price) 
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", trade_v: " + std::to_string(trade_volume)
                        + ", total_v: " + total_volume.get_str_value()
                        + ", cur_a: " + std::to_string(trade_amout)
                        + ", total_a: " + total_amount.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", bid;");
                break;
            }
        }
    }

    if( total_volume < volume )
    {
        LOG_WARN("_calc_otc_by_volume failed depth_sum_volume is: " 
                + total_volume.get_str_value() 
                + ", request_volume: " + std::to_string(volume));
        
        return QuoteResponse_Result_NOT_ENOUGH_VOLUME;
    }
        
    price = total_amount.get_value() / total_volume.get_value();
    double ori_price = price.get_value();

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
            price -= config.OtcOffset;
        } else {
            price += config.OtcOffset;
        }        
    }

    LOG_DEBUG("ori_price: " + std::to_string(ori_price) 
            + ", bias_price: " + price.get_str_value() 
            + ", bias_kind: " + std::to_string(config.OTCOffsetKind)
            + ", bias_value: " + std::to_string(config.OtcOffset));
    price.scale(precise, is_ask);
    return QuoteResponse_Result_OK;
}

QuoteResponse_Result _calc_otc_by_amount(const map<SDecimal, SInnerDepth>& depths, bool is_ask, QuoteConfiguration& config, double otc_amount, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0;
    SDecimal total_amount = 0;
    if( is_ask ) 
    {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, true);

            SDecimal cur_amounts = iter->second.total_volume * price;
            if( (total_amount + cur_amounts) <= otc_amount ) {
                total_volume += iter->second.total_volume;
                total_amount += cur_amounts;

                LOG_DEBUG("cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", ask;");
            } else {
                total_volume += (otc_amount - total_amount.get_value()) / price;
                total_amount = otc_amount;

                LOG_DEBUG("done cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", ask;");
                break;
            }        
        }
    } 
    else 
    {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, false);
            SDecimal cur_amounts = iter->second.total_volume * price;
            if( (total_amount + cur_amounts) <= otc_amount ) {
                total_volume += iter->second.total_volume;
                total_amount += cur_amounts;

                LOG_DEBUG("cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", bid;");                
            } else {
                total_volume += (otc_amount - total_amount.get_value()) / price;
                total_amount = otc_amount;
                LOG_DEBUG("done cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", bid;");
                break;
            }
        }
    }
    if( total_amount < otc_amount )
    {
        LOG_WARN("_calc_otc_by_amount failed depth_sum_volume is: " 
                + total_amount.get_str_value() 
                + ", otc_amount: " + std::to_string(otc_amount));
        return QuoteResponse_Result_NOT_ENOUGH_AMOUNT;
    }
        

    price = total_amount.get_value() / total_volume.get_value();
    double ori_price = price.get_value();

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

    LOG_DEBUG("ori_price: " + std::to_string(ori_price) 
            + ", bias_price: " + price.get_str_value() 
            + ", bias_kind: " + std::to_string(config.OTCOffsetKind)
            + ", bias_value: " + std::to_string(config.OtcOffset));

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

QuoteResponse_Result DataCenter::otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double volume, double amount, SDecimal& price)
{

    string direction_str = direction==QuoteRequest_Direction_BUY?"buy":"sell";
    LOG_INFO("[otc_query] " + exchange + "." + symbol 
            + ", direction=" + direction_str
            + ", volume=" + std::to_string(volume)
            + ", amount=" + std::to_string(amount));

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    auto iter = last_datas_.find(symbol);

    if(iter == last_datas_.end())
    {
        LOG_WARN("OTC Request Symbol " + symbol + " has no data");
        return QuoteResponse_Result_WRONG_SYMBOL;
    }
        
    SInnerQuote& quote = iter->second;

    LOG_DEBUG("\n OTC Quote Data \n" + quote_str(quote));

    if( volume > 0 )
    {
        
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_volume(quote.asks, true, params_.cache_config[symbol], volume, price, quote.precise);
        } else {
            return _calc_otc_by_volume(quote.bids, false, params_.cache_config[symbol], volume, price, quote.precise);   
        }
    } 
    else
    {
        
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_amount(quote.asks, true, params_.cache_config[symbol], amount, price, quote.precise);
        } else { 
            return _calc_otc_by_amount(quote.bids, false, params_.cache_config[symbol], amount, price, quote.precise);
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
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
}
