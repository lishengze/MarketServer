#include "risk_controller_config.h"
#include "datacenter.h"
#include "grpc_server.h"
#include "converter.h"

bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void _filter_depth_by_watermark(map<SDecimal, SInnerDepth>& src_depths, const SDecimal& watermark, bool is_ask)
{
    if( is_ask )
    {
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
                // cout << "delete ask depth " << price.get_str_value() << endl;
            } else {
                // 保留价位
                if( !patched ) {
                    patched = true;
                    
                    SInnerDepth fake;
                    for( const auto &v : volumes ) {
                        fake.exchanges[v.first] = v.second;
                    }

                    depth.mix_exchanges(fake, 0);
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
                // cout << "delete bid depth " << price.get_str_value() << endl;
            } else {
                // 保留价位
                if( !patched ) {
                    patched = true;
                    
                    SInnerDepth fake;
                    for( const auto &v : volumes ) {
                        fake.exchanges[v.first] = v.second;
                    }

                    depth.mix_exchanges(fake, 0);
                } else {
                    break;
                }
                // cout << "accumulate to depth " << price.get_str_value() << endl;
                v++;
            }
        }
    }
}

void _filter_by_watermark(SInnerQuote& src, const SDecimal& watermark)
{
    _filter_depth_by_watermark(src.asks, watermark, true);
    _filter_depth_by_watermark(src.bids, watermark, false);
}

void _calc_depth_bias(const vector<pair<SDecimal, SInnerDepth>>& depths, double price_bias, double volume_bias, bool is_ask, map<SDecimal, SInnerDepth>& dst) 
{
    for( const auto& v: depths )
    {
        SDecimal scaledPrice = v.first;
        if( is_ask ) {
            scaledPrice *= ( 1 + price_bias * 1.0 / 100);
        } else {
            if( price_bias < 100 )
                scaledPrice *= ( 1 - price_bias * 1.0 / 100);
            else
                scaledPrice = 0;
        }
        dst[scaledPrice].mix_exchanges(v.second, volume_bias * (-1));
    }
}

SInnerQuote& QuoteBiasWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    // 获取配置
    double price_bias = 0;
    double volume_bias = 0;
    auto iter = ctx.params.cache_config.find(src.symbol);
    if( iter != ctx.params.cache_config.end() ) {
        price_bias = iter->second.PriceBias;
        volume_bias = iter->second.VolumeBias;
    }

    std::cout << src.symbol << " price_bias: " << price_bias << ", volume_bias: " << volume_bias << std::endl;

    // 风控的价格和成交量处理
    SInnerQuote tmp;
    vector<pair<SDecimal, SInnerDepth>> depths;
    src.get_asks(depths);
    _calc_depth_bias(depths, price_bias, volume_bias, true, tmp.asks);
    src.asks.swap(tmp.asks);
    src.get_bids(depths);
    _calc_depth_bias(depths, price_bias, volume_bias, false, tmp.bids);
    src.bids.swap(tmp.bids);
    //tfm::printfln("QuoteBiasWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());

    return src;
}

WatermarkComputerWorker::WatermarkComputerWorker() 
{
    thread_run_ = true;
    thread_loop_ = new std::thread(&WatermarkComputerWorker::_calc_watermark, this);
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

    while( thread_run_ ) {
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
                //for( const auto& v : asks ) {
                    // cout << "ask " << v.get_str_value() << endl;
                //}
                sort(bids.begin(), bids.end());
                //for( const auto& v : bids ) {
                    // cout << "bid " << v.get_str_value() << endl;
                //}
                if( asks.size() > 0 && bids.size() > 0 ) {
                    iter->second->watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;
                    // cout << asks[asks.size()/2].get_str_value() << " " << bids[bids.size()/2].get_str_value() << " " << iter->second->watermark.get_str_value() << endl;
                }
            }
        }

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
};
   
SInnerQuote& WatermarkComputerWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
    set_snap(src);

    SDecimal watermark;
    get_watermark(src.symbol, watermark);
    _filter_by_watermark(src, watermark);
    // _log_and_print("worker(watermark)-%s: %s %lu/%lu", src.symbol.c_str(), watermark.get_str_value().c_str(), src.asks.size(), src.bids.size());
    
    //tfm::printfln("WatermarkComputerWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());
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
        hedge_percent = iter->second.HedgePercent;
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
    //tfm::printfln("AccountAjdustWorker %s %u/%u", src.symbol, src.asks.size(), src.bids.size());
    return src;
}

SInnerQuote& OrderBookWorker::process(SInnerQuote& src, PipelineContent& ctx)
{
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
    //std::cout << "publish for broker(raw) " << quote.symbol << " " << quote.ask_length << "/"<< quote.bid_length << std::endl;
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

    std::cout << "\nDataCenter::change_configuration  " << std::endl;

    for (auto iter:params_.cache_config)
    {
        std::cout << iter.first << "  PriceBias: " << iter.second.PriceBias  << ", VolumeBias: " << iter.second.VolumeBias << std::endl;
    }    
    _push_to_clients();
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

    std::cout << "\nparams_.cache_config  " << std::endl;

    for (auto iter:params_.cache_config)
    {
        std::cout << iter.first << "  PriceBias: " << iter.second.PriceBias  << ", VolumeBias: " << iter.second.VolumeBias << std::endl;
    }

    pipeline_.run(quote, params_, newQuote);

    // 检查是否发生变化
    auto iter = last_datas_.find(quote.symbol);
    if( iter != last_datas_.end() ) {
        const SInnerQuote& last_quote = iter->second;
        if( quote.time_origin <= last_quote.time_origin ) {
            //tfm::printfln("%s %ul %ul", quote.symbol, quote.time_origin, last_quote.time_origin);
            return;
        }
    }

    //std::cout << "publish(raw) " << quote.symbol << " " << newQuote.asks.size() << "/"<< newQuote.bids.size() << std::endl;
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd2(newQuote, ptrData.get(), true);    
    //std::cout << "publish " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;
    //_log_and_print("publish %s.%s %u/%u", quote.exchange, quote.symbol, ptrData->asks_size(), ptrData->bids_size());
    for( const auto& v : callbacks_) 
    {
        v->publish4Broker(quote.symbol, ptrData, NULL);
    }

    // send to clients
    std::shared_ptr<MarketStreamDataWithDecimal> ptrData2(new MarketStreamDataWithDecimal);
    innerquote_to_msd3(newQuote, ptrData2.get(), true);   
    for( const auto& v : callbacks_) 
    {
        v->publish4Client(quote.symbol, ptrData2, NULL);
    }

    last_datas_[quote.symbol] = newQuote;
}

QuoteResponse_Result _calc_otc_by_volume(const map<SDecimal, SInnerDepth>& depths, bool is_ask, const double& bias, double volume, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0, total_amount;
    if( is_ask ) {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * iter->first.get_value();
            } else {
                total_amount += (volume - total_volume.get_value()) * iter->first.get_value();
                total_volume = volume;
            }
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * iter->first.get_value();
            } else {
                total_amount += (volume - total_volume.get_value()) * iter->first.get_value();
                total_volume = volume;
            }
        }
    }

    if( total_volume < volume )
        return QuoteResponse_Result_NOT_ENOUGH_VOLUME;

    price = total_amount.get_value() / total_volume.get_value();
    if( is_ask ) {
        price *= ( 1 + bias * 1.0 / 100); 
    } else {
        price *= ( 1 - bias * 1.0 / 100); 
    }
    price.scale(precise, is_ask);
    return QuoteResponse_Result_OK;
}

QuoteResponse_Result _calc_otc_by_amount(const map<SDecimal, SInnerDepth>& depths, bool is_ask, const double& bias, double amount, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0, total_amount;
    if( is_ask ) {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            SDecimal amounts = iter->second.total_volume * iter->first.get_value();
            if( (total_amount + amounts) <= amount ) {
                total_volume += iter->second.total_volume;
                total_amount += amounts;
            } else {
                total_volume += (amount - total_amount.get_value()) / iter->first.get_value();
                total_amount = amount;
                break;
            }
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            SDecimal amounts = iter->second.total_volume * iter->first.get_value();
            if( (total_amount + amounts) <= amount ) {
                total_volume += iter->second.total_volume;
                total_amount += amounts;
            } else {
                total_volume += (amount - total_amount.get_value()) / iter->first.get_value();
                total_amount = amount;
                break;
            }
        }
    }
    if( total_amount < amount )
        return QuoteResponse_Result_NOT_ENOUGH_AMOUNT;

    price = total_amount.get_value() / total_volume.get_value();
    if( is_ask ) {
        price *= ( 1 + bias * 1.0 / 100); 
    } else {
        price *= ( 1 - bias * 1.0 / 100); 
    }
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
    _log_and_print("[otc_query] %s.%s direction=%s volume=%s amount=%s", exchange, symbol, direction, volume, amount);
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    auto iter = last_datas_.find(symbol);
    if( iter == last_datas_.end() )
        return QuoteResponse_Result_WRONG_SYMBOL;

    SInnerQuote& quote = iter->second;
    if( volume > 0 )
    {
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_volume(quote.asks, true, params_.cache_config[symbol].OtcBias, volume, price, quote.precise);
        } else {
            return _calc_otc_by_volume(quote.bids, false, params_.cache_config[symbol].OtcBias, volume, price, quote.precise);   
        }
    } 
    else
    {
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_amount(quote.asks, true, params_.cache_config[symbol].OtcBias, amount, price, quote.precise);
        } else { 
            return _calc_otc_by_amount(quote.bids, false, params_.cache_config[symbol].OtcBias, amount, price, quote.precise);
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