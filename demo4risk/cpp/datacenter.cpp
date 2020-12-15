#include "risk_controller_config.h"
#include "datacenter.h"
#include "grpc_server.h"


bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void _filter_depth_by_watermark(const map<SDecimal, SInnerDepth>& src_depths, const SDecimal& watermark, map<SDecimal, SInnerDepth>& dst_depths, bool is_ask)
{
    bool patched = false;
    unordered_map<TExchange, SDecimal> volumes;   // 被watermark滤掉的单量自动归到买卖一

    for( const auto& v : src_depths )
    {
        const SDecimal& price = v.first;
        if( is_ask ? (price <= watermark) : (price >= watermark) ) {
            // 过滤价位
            for( const auto& v2 : v.second.exchanges ){
                volumes[v2.first] += v2.second;
            }
        } else {
            // 保留价位
            dst_depths[v.first] = v.second;
            if( !patched ) {
                patched = true;
                
                SInnerDepth fake;
                for( const auto &v : volumes ) {
                    fake.exchanges[v.first] = v.second;
                }

                dst_depths[v.first].mix_exchanges(fake, 0);
            }
        }
    }
}

void _filter_by_watermark(const SInnerQuote& src, const SDecimal& watermark, SInnerQuote& dst)
{
    dst.symbol = src.symbol;
    _filter_depth_by_watermark(src.asks, watermark, dst.asks, true);
    _filter_depth_by_watermark(src.bids, watermark, dst.bids, false);
}

WatermarkComputerWorker::WatermarkComputerWorker() 
{
    thread_loop_ = new std::thread(&WatermarkComputerWorker::_calc_watermark, this);
}

WatermarkComputerWorker::~WatermarkComputerWorker() 
{
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void WatermarkComputerWorker::set_snap(const SInnerQuote& quote) 
{
    // 提取各交易所的买卖一
    unordered_map<TExchange, SDecimal> first_ask, first_bid;
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++ ) {
        for( const auto& v : iter->second.exchanges ) {
            const TExchange& exchange = v.first;
            if( first_ask.find(exchange) == first_ask.end() ) {
                first_ask[exchange] = iter->first;
            }
        }
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {
        for( const auto& v : iter->second.exchanges ) {
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

void WatermarkComputerWorker::_calc_watermark() {

    while( true ) {
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
                sort(bids.begin(), bids.end());
                if( asks.size() > 0 && bids.size() > 0 ) {
                    iter->second->watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;
                }
            }
        }

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};
   
SInnerQuote* WatermarkComputerWorker::process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out)
{
    vassign(out->symbol, src->symbol);
    vassign(out->time, src->time);
    vassign(out->time_arrive, src->time_arrive);
    vassign(out->seq_no, src->seq_no);

    SDecimal watermark;
    get_watermark(src->symbol, watermark);
    _filter_by_watermark(*src, watermark, *out);
    if( ctx.is_sample_ ) {
        _log_and_print("worker(watermark)-%s: %s %lu/%lu", src->symbol.c_str(), watermark.get_str_value().c_str(), out->asks.size(), out->bids.size());
    }
    return out;
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

SInnerQuote* AccountAjdustWorker::process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out)
{
    // 获取币种出现次数
    string sell_currency, buy_currency;
    int sell_count, buy_count;
    if( !get_currency(*src, sell_currency, sell_count, buy_currency, buy_count) )
        return src;

    // 动态调整每个品种的资金分配量
    unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    ctx.params.cache_account.get_hedge_amounts(sell_currency, ctx.params.cache_config.HedgePercent / sell_count, sell_total_amounts);
    ctx.params.cache_account.get_hedge_amounts(buy_currency, ctx.params.cache_config.HedgePercent / buy_count, buy_total_amounts);

    if( ctx.is_sample_ ) {
        _log_and_print("worker(account)-%s: sell %s %d", src->symbol.c_str(), sell_currency.c_str(), sell_count);
        for( const auto& v : sell_total_amounts ) {
            _log_and_print("worker(account)-%s: sell %s %.03f", src->symbol.c_str(), v.first.c_str(), v.second);
        }
        _log_and_print("worker(account)-%s: buy %s %d", src->symbol.c_str(), buy_currency.c_str(), buy_count);
        for( const auto& v : buy_total_amounts ) {
            _log_and_print("worker(account)-%s: buy  %s %.03f", src->symbol.c_str(), v.first.c_str(), v.second);
        }
    }

    // 逐档从总余额中扣除资金消耗
    for( auto iter = src->asks.begin() ; iter != src->asks.end() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        depth.total_volume = SDecimal::parse("0");
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second;
            double remain_amount = sell_total_amounts[exchange];
            if( remain_amount < need_amount.get_value() ) {
                iter2->second = SDecimal::parse("0");
            } else {
                sell_total_amounts[exchange] -= need_amount.get_value();
                depth.total_volume += need_amount;
            }
        }
    }
    for( auto iter = src->bids.rbegin() ; iter != src->bids.rend() ; iter++ ) 
    {
        SInnerDepth& depth = iter->second;
        depth.total_volume = SDecimal::parse("0");
        for( auto iter2 = depth.exchanges.begin() ; iter2 != depth.exchanges.end() ; iter2++ ) {
            const TExchange& exchange = iter2->first;
            const SDecimal& need_amount = iter2->second * iter->first.get_value();
            double remain_amount = sell_total_amounts[exchange];
            if( remain_amount < need_amount.get_value() ) {
                iter2->second = SDecimal::parse("0");
            } else {
                sell_total_amounts[exchange] -= need_amount.get_value();
                depth.total_volume += need_amount;
                //cout << remain_amount << " " << iter2->second.get_str_value() << " " << iter->first.get_str_value() << " " << depth.total_volume.get_str_value() << endl;
            }
        }
    }
    return src;
}

SInnerQuote* OrderBookWorker::process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out)
{
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

void innerquote_to_msd2(const SInnerQuote& quote, MarketStreamData* msd, bool check_total_volume) 
{
    msd->set_symbol(quote.symbol);
    msd->set_is_snap(true);
    //msd->set_time(quote.time);
    //msd->set_time_arrive(quote.time_arrive);
    //char sequence[256];
    //sprintf(sequence, "%lld", quote.seq_no);
    //msd->set_msg_seq(sequence);
    // 卖盘
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        Depth* depth = msd->add_asks();        
        depth->set_price(iter->first.get_str_value());
        for( const auto& v : iter->second.exchanges ) {
            if( v.second.is_zero() )
                continue;
             (*depth->mutable_data())[v.first] = v.second.get_value();
        }
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        Depth* depth = msd->add_bids();        
        depth->set_price(iter->first.get_str_value());
        for( const auto& v : iter->second.exchanges ) {
            if( v.second.is_zero() )
                continue;
             (*depth->mutable_data())[v.first] = v.second.get_value();
        }
    }
}

void innerquote_to_msd3(const SInnerQuote& quote, MarketStreamDataWithDecimal* msd, bool check_total_volume) 
{
    msd->set_symbol(quote.symbol);
    msd->set_is_snap(true);
    //msd->set_time(quote.time);
    //msd->set_time_arrive(quote.time_arrive);
    //char sequence[256];
    //sprintf(sequence, "%lld", quote.seq_no);
    //msd->set_msg_seq(sequence);
    // 卖盘
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        DepthWithDecimal* depth = msd->add_asks();
        set_decimal(depth->mutable_price(), iter->first);
        set_decimal(depth->mutable_volume(), iter->second.total_volume);
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        DepthWithDecimal* depth = msd->add_bids();
        set_decimal(depth->mutable_price(), iter->first);
        set_decimal(depth->mutable_volume(), iter->second.total_volume);
    }
}

/////////////////////////////////////////////////////////////////////////////////
DataCenter::DataCenter() {
    pipeline_.add_worker(&watermark_worker_);
    pipeline_.add_worker(&account_worker_);
    pipeline_.add_worker(&orderbook_worker_);
}

DataCenter::~DataCenter() {

}

void _calc_depth_bias(const vector<pair<SDecimal, SInnerDepth>>& depths, double price_bias, double volume_bias, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    for( const auto& v: depths )
    {
        SDecimal scaledPrice = v.first;
        if( is_ask ) {
            scaledPrice *= ( 1 + price_bias * 1.0 / 100); 
        } else {
            scaledPrice *= ( 1 - price_bias * 1.0 / 100); 
        }
        dst[scaledPrice].mix_exchanges(v.second, volume_bias);
    }
}

void DataCenter::_add_quote(const SInnerQuote& src, SInnerQuote& dst, Params& params)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

    // 取出参数
    params = params_;

    // 计算价位和成交量偏移
    dst.symbol = src.symbol;

    vector<pair<SDecimal, SInnerDepth>> depths;
    src.get_asks(depths);
    _calc_depth_bias(depths, params_.cache_config.PriceBias, params_.cache_config.VolumeBias, true, dst.asks);
    src.get_bids(depths);
    _calc_depth_bias(depths, params_.cache_config.PriceBias, params_.cache_config.VolumeBias, false, dst.bids);
    
    // 加入快照
    datas_[src.symbol] = dst;
}

void DataCenter::add_quote(const SInnerQuote& quote)
{    
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    //std::cout << "publish for broker(raw) " << quote.symbol << " " << quote.ask_length << "/"<< quote.bid_length << std::endl;
    innerquote_to_msd2(quote, ptrData.get(), false);
    //std::cout << "publish for broker " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;
    PUBLISHER->publish4Hedge(quote.symbol, ptrData, NULL);

    // 更新内存中的行情
    Params params;
    SInnerQuote new_quote;
    _add_quote(quote, new_quote, params);

    // 加入买卖一
    watermark_worker_.set_snap(new_quote);
    account_worker_.set_snap(new_quote);

    // 发布行情
    _publish_quote(new_quote, params);
};

void DataCenter::change_account(const AccountInfo& info)
{
    cout << "change_account" << endl;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_account = info;
    }

    _push_to_clients();
}

void DataCenter::change_configuration(const QuoteConfiguration& config)
{
    cout << "change_configuration" << endl;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_config = config;
    }

    _push_to_clients();
}

void DataCenter::change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
    cout << "change_orders" << endl;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_order[symbol] = make_pair(asks, bids);
    }

    _push_to_clients(symbol);
}

void DataCenter::_push_to_clients(const TSymbol& symbol) 
{
    Params params;
    // 获取参数
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params = params_;
    }

    if( symbol == "" ) 
    {
        for( auto iter = datas_.begin() ; iter != datas_.end() ; ++iter ) 
        {
            _publish_quote(iter->second, params);
        }
    } else {
        auto iter = datas_.find(symbol);
        if( iter == datas_.end() )
            return;

        _publish_quote(iter->second, params);
    }
}

void DataCenter::_publish_quote(const SInnerQuote& quote, const Params& params) 
{    
    SInnerQuote newQuote;
    pipeline_.run(quote, params, newQuote);

    // 检查是否发生变化
    auto iter = last_datas_.find(quote.symbol);
    if( iter != last_datas_.end() ) {
        const SInnerQuote& last_quote = iter->second;
        if( memcmp(&last_quote, &quote, sizeof(SInnerQuote)) == 0 )
            return;
    }

    std::cout << "publish(raw) " << quote.symbol << " " << newQuote.asks.size() << "/"<< newQuote.bids.size() << std::endl;
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd2(newQuote, ptrData.get(), true);    
    std::cout << "publish " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;
    PUBLISHER->publish4Broker(quote.symbol, ptrData, NULL);

    // send to clients
    std::shared_ptr<MarketStreamDataWithDecimal> ptrData2(new MarketStreamDataWithDecimal);
    innerquote_to_msd3(newQuote, ptrData2.get(), true);   
    PUBLISHER->publish4Client(quote.symbol, ptrData2, NULL);

    last_datas_[quote.symbol] = newQuote;
}
