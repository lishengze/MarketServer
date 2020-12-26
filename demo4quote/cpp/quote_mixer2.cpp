#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "converter.h"

void update_depth_diff(const map<SDecimal, SDecimal>& update, map<SDecimal, SDecimal>& dst)
{
    for( const auto& v : update ) {
        if( v.second.is_zero() ) {
            auto iter = dst.find(v.first);
            if( iter != dst.end() ) {
                dst.erase(iter);
            }
        } else {
            dst[v.first] = v.second;
        }
    }
}

void QuoteCacher::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    std::shared_ptr<TradeWithDecimal> pub_trade = std::make_shared<TradeWithDecimal>();
    pub_trade->set_exchange(exchange);
    pub_trade->set_symbol(symbol);
    //pub_trade->set_time(trade);
    
    for( const auto& v : callbacks_) 
    {
        v->publish_trade(exchange, symbol, pub_trade);
    }
}

void QuoteCacher::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    //_log_and_print("%s mix_frequency=%.03f", symbol.c_str(), frequency);
    std::unique_lock<std::mutex> l{ mutex_config_ };
    configs_[symbol] = config;
}

void QuoteCacher::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    SSymbolConfig config;
    {
        std::unique_lock<std::mutex> l{ mutex_config_ };
        config = configs_[symbol];
    }

    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        singles_[symbol][exchange] = quote;
    }
    
    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap = depth_to_pbquote2(exchange, symbol, quote, config.depths[exchange], true);
    for( const auto& v : callbacks_) 
    {
        v->publish_single(exchange, symbol, pub_snap, NULL);
    }

    mixer_->on_snap(exchange, symbol, quote);

    /*
    // 更新单个品种 & 手续费率和精度处理
    SDepthQuote cpsQuote;
    this->_snap_singles_(exchange, symbol, quote, cpsQuote);

    // 更新内存中的行情
    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap;
    if( !_on_snap(exchange, symbol, quote, pub_snap) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, NULL, true);*/
}

void QuoteCacher::clear_exchange(const TExchange& exchange)
{    
    /*
    unordered_map<TSymbol, std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( auto&v : singles_ ) {
            //_println_("QuoteMixer2 clear exchange %s: %s...", exchange.c_str(), v.first.c_str());
            v.second->asks = _clear_exchange(exchange, v.second->asks);
            v.second->bids = _clear_exchange(exchange, v.second->bids);
            //_println_("QuoteMixer2 clear exchange %s: %s after clear ...", exchange.c_str(), v.first.c_str());
            snaps[v.first] = mixquote_to_pbquote2("", v.first, v.second, _compute_params_[v.first].depth, true);
            //_println_("QuoteMixer2 clear exchange %s: %s", exchange.c_str(), v.first.c_str());
        }
    }

    for( const auto& v : callbacks_) 
    {
        v->publish_single(exchange, symbol, pub_snap, NULL);
    }*/
}

void QuoteCacher::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& update) 
{
    SSymbolConfig config;
    {
        std::unique_lock<std::mutex> l{ mutex_config_ };
        config = configs_[symbol];
    }

    SDepthQuote snap;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        SDepthQuote& cache = singles_[symbol][exchange];
        update_depth_diff(update.asks, cache.asks);        
        update_depth_diff(update.bids, cache.bids);
        snap = cache;
    }

    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap, pub_diff;
    pub_snap = depth_to_pbquote2(exchange, symbol, snap, config.depths[exchange], true);
    pub_diff = depth_to_pbquote2(exchange, symbol, update, config.depths[exchange], false);
    for( const auto& v : callbacks_) 
    {
        v->publish_single(exchange, symbol, pub_snap, pub_diff);
    }

    mixer_->on_snap(exchange, symbol, snap);
    /*
    // 更新单个品种 & 手续费率和精度处理
    SDepthQuote cpsQuote;
    this->_update_singles_(exchange, symbol, quote, cpsQuote);
    //cpsQuote.print();

    // 更新内存中的行情
    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap;
    if( !_on_snap(exchange, symbol, cpsQuote, pub_snap) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, NULL, true);
    */
}

/////////////////////////////////////////////////////////////////////
void process_depths(const map<SDecimal, SDecimal>& src, map<SDecimal, SDecimal>& dst, int precise, const SymbolFee& fee, bool is_ask)
{
    if( is_ask ) {
        SDecimal lastPrice = SDecimal::min_decimal();
        for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
        {
            // 卖价往上取整
            SDecimal scaledPrice;
            fee.compute(iter->first, scaledPrice, true);
            //cout << iter->first.get_str_value() << " " << precise << " " << scaledPrice.value << " " << scaledPrice.base << endl;
            scaledPrice.scale(precise, true);
            //scaledPrice.from(scaledPrice, precise, true); 
            //cout << iter->first.get_str_value() << " " << precise << " " << scaledPrice.value << " " << scaledPrice.base << endl;

            bool is_new_price = scaledPrice > lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice] = dst[lastPrice] + iter->second;
            }
        }
    } else {
        SDecimal lastPrice = SDecimal::max_decimal();
        for( auto iter = src.rbegin() ; iter != src.rend() ; iter++ ) 
        {
            // 买价往下取整
            SDecimal scaledPrice;
            fee.compute(iter->first, scaledPrice, false);
            scaledPrice.scale(precise, false);
            //scaledPrice.from(scaledPrice, precise, false); 

            bool is_new_price = scaledPrice < lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice] = dst[lastPrice] + iter->second;
            }
        }
    }
}
bool QuoteMixer2::_check_update_clocks(const TSymbol& symbol, float frequency) {
    if( frequency == 0 )
        return true;

    std::unique_lock<std::mutex> l{ mutex_clocks_ };
    // 每秒更新频率控制
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/frequency) )
    {
        return false;
    }
    last_clocks_[symbol] = get_miliseconds();
    return true;
}

void QuoteMixer2::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    SSymbolConfig config;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        config = configs_[symbol];
    }

    SDepthQuote output;
    output.exchange = exchange;
    output.symbol = symbol;
    output.arrive_time = quote.arrive_time;
    output.sequence_no = quote.sequence_no;
    cout << exchange << " " << config.precise << " " << config.vprecise << endl;
    process_depths(quote.asks, output.asks, config.precise, config.fees[exchange], true);
    process_depths(quote.bids, output.bids, config.precise, config.fees[exchange], false);

    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) ) {
            ptr = new SMixQuote();
            quotes_[symbol] = ptr;
        }

        _inner_process(exchange, symbol, output, ptr);

        if( !_check_update_clocks(symbol, config.frequency) ) {
            return;
        }

        pub_snap = mixquote_to_pbquote2("", symbol, ptr, config.depth, true);
    }
    
    std::cout << "publish(snap) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    for( const auto& v : callbacks_) 
    {
        v->publish_mix(symbol, pub_snap, NULL);
    }
}

void QuoteMixer2::clear_exchange(const TExchange& exchange)
{    
    /*()
    unordered_map<TSymbol, std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( auto&v : quotes_ ) 
        {
            //_println_("QuoteMixer2 clear exchange %s: %s...", exchange.c_str(), v.first.c_str());
            v.second->asks = _clear_exchange(exchange, v.second->asks);
            v.second->bids = _clear_exchange(exchange, v.second->bids);
            //_println_("QuoteMixer2 clear exchange %s: %s after clear ...", exchange.c_str(), v.first.c_str());
            snaps[v.first] = mixquote_to_pbquote2("", v.first, v.second, _compute_params_[v.first].depth, true);
            //_println_("QuoteMixer2 clear exchange %s: %s", exchange.c_str(), v.first.c_str());
        }
    }

    for( auto& snap : snaps ) 
    {
        for( const auto& v : callbacks_) 
        {
            v->publish_mix(snap.first, snap.second, NULL);
        }
    }*/
}

void QuoteMixer2::_inner_process(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SMixQuote* ptr)
{
    // 1. 清除老的exchange数据
    ptr->asks = _clear_exchange(exchange, ptr->asks);
    ptr->bids = _clear_exchange(exchange, ptr->bids);

    // 2. 合并价位
    vector<pair<SDecimal, SDecimal>> depths;
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++ ) {
        depths.push_back(make_pair(iter->first, iter->second));
    }
    ptr->asks = _mix_exchange(exchange, ptr->asks, depths, true);

    depths.clear();
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++ ) {
        depths.push_back(make_pair(iter->first, iter->second));        
    }
    ptr->bids = _mix_exchange(exchange, ptr->bids, depths, false);
    ptr->sequence_no = quote.sequence_no;
}

bool QuoteMixer2::_get_quote(const TSymbol& symbol, SMixQuote*& ptr) const {
    auto iter = quotes_.find(symbol);
    if( iter == quotes_.end() )
        return false;
    ptr = iter->second;
    return true;
}

SMixDepthPrice* QuoteMixer2::_clear_exchange(const TExchange& exchange, SMixDepthPrice* depths) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;
    while( tmp != NULL ) {
        unordered_map<TExchange, SDecimal>& volumeByExchange = tmp->volume;
        auto iter = volumeByExchange.find(exchange);
        if( iter != volumeByExchange.end() ) {
            volumeByExchange.erase(iter);                
            if( volumeByExchange.size() == 0 ) {
                // 删除             
                SMixDepthPrice* waitToDel = tmp;
                last->next = tmp->next;
                tmp = tmp->next;
                delete waitToDel;
                continue;
            }
        }
        last = tmp;
        tmp = tmp->next;
    }
    return head.next;
}

SMixDepthPrice* QuoteMixer2::_mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, SDecimal>>& depths, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;
    SMixDepthPrice* tmp = mixedDepths;

    // 1. 混合
    auto iter = depths.begin();
    for( tmp = head.next ; iter != depths.end() && tmp != NULL ; ) {
        const SDecimal& price = iter->first;
        const SDecimal& volume = iter->second;
        if( volume < VOLUME_PRECISE ) {
            iter++;
            continue;
        }
        if( isAsk ? (price < tmp->price) : (price > tmp->price) ) {
            // 新建价位
            SMixDepthPrice *newDepth = new SMixDepthPrice();
            newDepth->price = price;
            newDepth->volume[exchange] = volume;
            
            last->next = newDepth;
            last = newDepth;
            newDepth->next = tmp;
            iter++;
        } else if( price == tmp->price ) {
            tmp->volume[exchange] = volume;
            iter++;
        } else {
            last = tmp;
            tmp = tmp->next;
        }
    }

    // 2. 剩余全部加入队尾
    for( ; iter != depths.end() ; iter ++ ) {
        const SDecimal& price = iter->first;
        const SDecimal& volume = iter->second;
        if( volume < VOLUME_PRECISE ) {
            continue;
        }
        // 新建价位
        SMixDepthPrice *newDepth = new SMixDepthPrice();
        newDepth->price = price;
        newDepth->volume[exchange] = volume;
        newDepth->next = NULL;
        // 插入
        last->next = newDepth;
        last = newDepth;
    }

    // 3. 删除多余的价位
    tmp = head.next;
    while( tmp != NULL) {
        tmp = tmp->next;
    }
    if( tmp != NULL ) {
        SMixDepthPrice* deletePtr = tmp->next;
        tmp->next = NULL;
        while( deletePtr != NULL ) {
            SMixDepthPrice *waitToDelete = deletePtr;
            deletePtr = deletePtr->next;
            delete waitToDelete;
        }
    }

    return head.next;
}

void QuoteMixer2::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    //_log_and_print("%s mix_frequency=%.03f", symbol.c_str(), frequency);
    std::unique_lock<std::mutex> l{ mutex_config_ };
    configs_[symbol] = config;
}

