#include "risk_controller_config.h"
#include "datacenter.h"
#include "grpc_server.h"

WatermarkComputer::WatermarkComputer() 
{
    thread_loop_ = new std::thread(&WatermarkComputer::_calc_watermark, this);
}

WatermarkComputer::~WatermarkComputer() 
{
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void WatermarkComputer::set_snap(const SInnerQuote& quote) 
{
    // 提取各交易所的买卖一
    unordered_map<TExchange, SDecimal> first_ask, first_bid;
    for( uint32 i = 0 ; i < quote.ask_length ; ++i ) {
        const SInnerDepth& depth = quote.asks[i];
        for( uint32 j = 0 ; j < depth.exchange_length ; ++j ) {
            const TExchange& exchange = depth.exchanges[j].name;
            if( first_ask.find(exchange) == first_ask.end() ) {
                first_ask[exchange] = depth.price;
            }
        }
    }
    for( uint32 i = 0 ; i < quote.bid_length ; ++i ) {
        const SInnerDepth& depth = quote.bids[i];
        for( uint32 j = 0 ; j < depth.exchange_length ; ++j ) {
            const TExchange& exchange = depth.exchanges[j].name;
            if( first_bid.find(exchange) == first_bid.end() ) {
                first_bid[exchange] = depth.price;
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

bool WatermarkComputer::get_watermark(const string& symbol, SDecimal& watermark) const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };    
    auto iter = watermark_.find(symbol);
    if( iter == watermark_.end() || iter->second->watermark.value == 0 ) {
        return false;
    }
    watermark = iter->second->watermark;
    return true;
};

void WatermarkComputer::_calc_watermark() {

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

bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void innerquote_to_msd(const SInnerQuote& quote, MarketStreamData* msd) 
{
    msd->set_symbol(quote.symbol);
    //msd->set_time(quote.time);
    //msd->set_time_arrive(quote.time_arrive);
    //char sequence[256];
    //sprintf(sequence, "%lld", quote.seq_no);
    //msd->set_msg_seq(sequence);
    // 卖盘
    int count = 0;
    for( uint32 i = 0 ; i < quote.ask_length && count < CONFIG->grpc_publish_depth_ ; ++i ) {
        const SInnerDepth& srcDepth = quote.asks[i];
        if( srcDepth.total_volume <= 0 )
            continue;
        count++;
        Depth* depth = msd->add_asks();        
        depth->set_price(srcDepth.price.get_str_value());
        depth->set_volume(srcDepth.total_volume);
        for( uint32 j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            if( srcDepth.exchanges[j].volume <= 0 )
                continue;
            (*depth->mutable_data())[srcDepth.exchanges[j].name] = srcDepth.exchanges[j].volume;
        }
    }
    // 买盘
    count = 0;
    for( uint32 i = 0 ; i < quote.bid_length && count < CONFIG->grpc_publish_depth_ ; ++i ) {
        const SInnerDepth& srcDepth = quote.bids[i];
        if( srcDepth.total_volume <= 0 )
            continue;
        count++;
        Depth* depth = msd->add_bids();        
        depth->set_price(srcDepth.price.get_str_value());
        depth->set_volume(srcDepth.total_volume);
        for( uint32 j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            if( srcDepth.exchanges[j].volume <= 0 )
                continue;
            (*depth->mutable_data())[srcDepth.exchanges[j].name] = srcDepth.exchanges[j].volume;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
DataCenter::DataCenter() {

}

DataCenter::~DataCenter() {

}

void _calc_depth_bias(const SInnerDepth* depths, const unsigned int& depth_length, double price_bias, double volume_bias, bool is_ask, SInnerDepth* dst, unsigned int& dst_length) 
{
    int count = 0;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    for( uint32 i = 0 ; i < depth_length ; ++i )
    {
        const SInnerDepth& level = depths[i];
        
        SDecimal scaledPrice;
        if( is_ask ) {
            scaledPrice.from(level.price * ( 1 + price_bias * 1.0 / 100), -1, true); 
        } else {
            scaledPrice.from(level.price * ( 1 - price_bias * 1.0 / 100), -1, false); 
        }

        if( is_ask ? (scaledPrice > lastPrice ) : (scaledPrice < lastPrice) ){
            count++;
            dst[count-1].price = scaledPrice;
            lastPrice = scaledPrice;
        }
        dst[count-1].mix_exchanges(level, volume_bias);
    }
    dst_length = count;
}

void DataCenter::_add_quote(const SInnerQuote& src, SInnerQuote& dst, Params& params)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    // 获取当前参数
    params = params_;

    // 计算币种出现的次数
    if( datas_.find(src.symbol) == datas_.end() ) {
        string sell_currency, buy_currency;
        if( getcurrency_from_symbol(src.symbol, sell_currency, buy_currency) ) {
            currency_count_[sell_currency] += 1;
            currency_count_[buy_currency] += 1;
        }
    }

    // 计算价位和成交量偏移
    strcpy(dst.symbol, src.symbol);
    _calc_depth_bias(src.asks, src.ask_length, params.cache_config.PriceBias, params.cache_config.VolumeBias, true, dst.asks, dst.ask_length);
    _calc_depth_bias(src.bids, src.bid_length, params.cache_config.PriceBias, params.cache_config.VolumeBias, false, dst.bids, dst.bid_length);
    
    // 加入快照
    datas_[src.symbol] = dst;
}

void DataCenter::add_quote(const SInnerQuote& quote)
{
    Params params;
    SInnerQuote new_quote;

    // 更新内存中的行情
    _add_quote(quote, new_quote, params);

    // 加入买卖一
    watermark_computer_.set_snap(new_quote);

    // 发布行情
    _publish_quote(quote, params);
};

void DataCenter::change_account(const AccountInfo& info)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_account = info;
    }

    _push_to_clients();
}

void DataCenter::change_configuration(const QuoteConfiguration& config)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_config = config;
    }

    _push_to_clients();
}

void DataCenter::change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
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
    _calc_newquote(quote, params, newQuote);

    std::cout << "publish(newQuote) " << quote.symbol << " " << newQuote.ask_length << "/"<< newQuote.bid_length << std::endl;

    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd(newQuote, ptrData.get());
    
    std::cout << "publish " << quote.symbol << " " << ptrData->asks_size() << "/"<< ptrData->bids_size() << std::endl;

    // send to clients
    PUBLISHER->publish4Hedge(quote.symbol, ptrData, NULL);
}

void _filter_depth_by_watermark(const SInnerDepth* src_depths, const uint32& src_depth_length, const SDecimal& watermark, SInnerDepth* dst_depths, uint32& dst_depth_length, bool is_ask)
{
    bool patched = false;
    unordered_map<TExchange, double> volumes;   // 被watermark滤掉的单量自动归到买卖一

    dst_depth_length = 0;
    for( uint32 i = 0 ; i < src_depth_length ; ++i )
    {
        const SInnerDepth& depth = src_depths[i];
        if( is_ask ? (depth.price <= watermark) : (depth.price >= watermark) ) {
            // 过滤价位
            for( uint32 j = 0 ; j < depth.exchange_length ; ++j ) {
                volumes[depth.exchanges[j].name] += depth.exchanges[j].volume;
            }
        } else {
            // 保留价位
            dst_depths[dst_depth_length] = depth;
            if( !patched ) {
                patched = true;
                
                SInnerDepth fake;
                uint32 count = 0;
                for( auto &v : volumes ) {
                    vassign(fake.exchanges[count].name, MAX_EXCHANGENAME_LENGTH, v.first);
                    fake.exchanges[count].volume = v.second;
                    count++;
                    if( count >= MAX_EXCHANGE_LENGTH )
                        break;
                }
                fake.exchange_length = count;

                dst_depths[dst_depth_length].mix_exchanges(fake, 0);
            }
            dst_depth_length ++;
        }
    }
}

void _filter_by_watermark(const SInnerQuote& src, const SDecimal& watermark, SInnerQuote& dst)
{
    strcpy(dst.symbol, src.symbol);
    _filter_depth_by_watermark(src.asks, src.ask_length, watermark, dst.asks, dst.ask_length, true);
    _filter_depth_by_watermark(src.bids, src.bid_length, watermark, dst.bids, dst.bid_length, false);
}

void DataCenter::_calc_newquote(const SInnerQuote& quote, const Params& params, SInnerQuote& newQuote)
{
    //newQuote = quote;
    //return;
    string symbol = quote.symbol;
    bool debug = false;
    if( symbol == "ETH_USDT") {
        debug = true;
    }

    // 按照水位过滤
    SDecimal watermark;
    watermark_computer_.get_watermark(symbol, watermark);
    _filter_by_watermark(quote, watermark, newQuote);
    if( debug ) {
        std::cout << std::fixed << "testdata:" << watermark.get_str_value()
            //<< " " << quote.asks[0].price.get_str_value() << "-" << quote.asks[quote.ask_length-1].price.get_str_value() << "/" << quote.bids[0].price.get_str_value() << "-" << quote.bids[quote.bid_length-1].price.get_str_value()
            << " " << newQuote.ask_length << "/" << newQuote.bid_length 
            << std::endl;
    }

    // 解析币种信息
    vassign(newQuote.symbol, MAX_SYMBOLNAME_LENGTH, quote.symbol);
    vassign(newQuote.time, quote.time);
    vassign(newQuote.time_arrive, quote.time_arrive);
    vassign(newQuote.seq_no, quote.seq_no);

    // 获取币种出现次数
    string sell_currency, buy_currency;
    if( !getcurrency_from_symbol(symbol, sell_currency, buy_currency) )
        return;
    auto iter = currency_count_.find(sell_currency);
    if( iter == currency_count_.end() )
        return;
    int sell_count = iter->second; // 卖币种出现次数
    auto iter2 = currency_count_.find(buy_currency);
    if( iter2 == currency_count_.end() )
        return;
    int buy_count = iter->second; // 买币种出现次数
    
    // 动态调整每个品种的资金分配量
    unordered_map<TExchange, double> sell_total_amounts, buy_total_amounts;
    params.cache_account.get_hedge_amounts(sell_currency, params.cache_config.HedgePercent / sell_count, sell_total_amounts);
    params.cache_account.get_hedge_amounts(buy_currency, params.cache_config.HedgePercent / buy_count, buy_total_amounts);

    // 逐档从总余额中扣除资金消耗
    for( uint32 i = 0 ; i < newQuote.ask_length ; ++i ) {
        SInnerDepth& depth = newQuote.asks[i];
        depth.total_volume = 0;
        for( uint32 j = 0 ; j < depth.exchange_length ; ++j ) {
            string exchange = depth.exchanges[j].name;
            double remain_amount = sell_total_amounts[exchange];
            double need_amount = depth.exchanges[j].volume; // 计算需要消耗的资金量
            if( debug ) {
                //std::cout << std::fixed << exchange << " " << sell_currency << " remain_amount:" << remain_amount << " need_amount:" << need_amount << std::endl;
            }
            if( remain_amount < need_amount ) {
                depth.exchanges[j].volume = 0;
            } else {
                sell_total_amounts[exchange] -= need_amount;
                depth.total_volume += depth.exchanges[j].volume;
            }
        }
    }
    for( uint32 i = 0 ; i < newQuote.bid_length ; ++i ) {
        SInnerDepth& depth = newQuote.bids[i];
        depth.total_volume = 0;
        for( uint32 j = 0 ; j < depth.exchange_length ; ++j ) {
            string exchange = depth.exchanges[j].name;
            double remain_amount = buy_total_amounts[exchange];
            double need_amount = depth.price.get_value() * depth.exchanges[j].volume; // 计算需要消耗的资金量
            if( remain_amount < need_amount ) {
                depth.exchanges[j].volume = 0;
            } else {
                buy_total_amounts[exchange] -= need_amount;
                depth.total_volume += depth.exchanges[j].volume;
            }
        }
    }

    // 加工后聚合行情扣减系统未对冲单的价位
    auto orderBookIter = params.cache_order.find(symbol);
    if( orderBookIter != params.cache_order.end() ) 
    {
        // 卖盘
        for( uint32 i = 0 ; i < newQuote.ask_length ; ) {
            const vector<SOrderPriceLevel>& levels = orderBookIter->second.first;
            for( auto iter = levels.begin() ; iter != levels.end() ; ) {
                if( iter->price < newQuote.asks[i].price ) {
                    iter ++;
                } else if( iter->price > newQuote.asks[i].price ) {
                    i++;
                } else {
                    newQuote.asks[i].total_volume -= iter->volume;
                }
            }
        }
        // 买盘
        for( uint32 i = 0 ; i < newQuote.bid_length ; ) {
            const vector<SOrderPriceLevel>& levels = orderBookIter->second.second;
            for( auto iter = levels.begin() ; iter != levels.end() ; ) {
                if( iter->price < newQuote.bids[i].price ) {
                    iter ++;
                } else if( iter->price > newQuote.bids[i].price ) {
                    i++;
                } else {
                    newQuote.bids[i].total_volume -= iter->volume;
                }
            }
        }

    }
}
