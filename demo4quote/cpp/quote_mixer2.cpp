#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "converter.h"

/////////////////////////////////////////////////////////////////////
QuoteMixer2::QuoteMixer2() {
}   

QuoteMixer2::~QuoteMixer2() {
}

bool QuoteMixer2::_check_update_clocks(const TSymbol& symbol, float frequency) {
    if( frequency == 0 )
        return true;

    std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
    // 每秒更新频率控制
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/frequency) )
    {
        return false;
    }
    last_clocks_[symbol] = get_miliseconds();
    return true;
}

void QuoteMixer2::_publish_quote(const TSymbol& symbol, std::shared_ptr<MarketStreamData> pub_snap, std::shared_ptr<MarketStreamData> pub_diff, bool is_snap) 
{
    if( is_snap ) {
        std::cout << "publish(snap) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    } else {
        std::cout << "publish(update) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    }
    PUBLISHER->publish_mix(symbol, pub_snap, pub_diff);
}

void QuoteMixer2::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    // 更新单个品种 & 手续费率和精度处理
    SDepthQuote cpsQuote;
    this->_snap_singles_(exchange, symbol, quote, cpsQuote);

    // 更新内存中的行情
    std::shared_ptr<MarketStreamData> pub_snap;
    if( !_on_snap(exchange, symbol, quote, pub_snap) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, NULL, true);
}

void QuoteMixer2::clear_exchange(const TExchange& exchange)
{    
    unordered_map<TSymbol, std::shared_ptr<MarketStreamData>> snaps;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        for( auto&v : quotes_ ) {
            //_println_("QuoteMixer2 clear exchange %s: %s...", exchange.c_str(), v.first.c_str());
            v.second->asks = _clear_exchange(exchange, v.second->asks);
            v.second->bids = _clear_exchange(exchange, v.second->bids);
            //_println_("QuoteMixer2 clear exchange %s: %s after clear ...", exchange.c_str(), v.first.c_str());
            snaps[v.first] = mixquote_to_pbquote2("", v.first, v.second, _compute_params_[v.first].depth, true);
            //_println_("QuoteMixer2 clear exchange %s: %s", exchange.c_str(), v.first.c_str());
        }
    }

    for( auto& v : snaps ) {
        _publish_quote(v.first, v.second, NULL, true);
    }
}

void QuoteMixer2::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    // 更新单个品种 & 手续费率和精度处理
    SDepthQuote cpsQuote;
    this->_update_singles_(exchange, symbol, quote, cpsQuote);

    // 更新内存中的行情
    std::shared_ptr<MarketStreamData> pub_snap;
    if( !_on_snap(exchange, symbol, cpsQuote, pub_snap) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, NULL, true);
}

void QuoteMixer2::_inner_process(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SMixQuote* ptr)
{
    // 1. 清除老的exchange数据
    ptr->asks = _clear_exchange(exchange, ptr->asks);
    ptr->bids = _clear_exchange(exchange, ptr->bids);

    // 2. 合并价位
    vector<pair<SDecimal, double>> depths;
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

bool QuoteMixer2::_on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) ) {
        ptr = new SMixQuote();
        quotes_[symbol] = ptr;
    } else {
        // 1. 清除老的exchange数据
        //ptr->asks = _clear_exchange(exchange, ptr->asks);
        //ptr->bids = _clear_exchange(exchange, ptr->bids);
    }

    _inner_process(exchange, symbol, quote, ptr);
    /*
    // 2. 合并价位
    vector<pair<SDecimal, double>> depths;
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
    */
    // 检查发布频率
    //if( symbol == "BTC_USDT" ) {
    //    std::cout << quote.exchange << " " << quote.ask_length << " " << quote.asks[0].price.get_str_value() << " " << 
    //    ptr->asks->price.get_str_value() << " " << ptr->asks->next->price.get_str_value() << " " << ptr->asks->next->next->price.get_str_value() << std::endl;
    //}
    if( !_check_update_clocks(symbol, publish_params_[symbol].frequency) ) {
        return false;
    }

    pub_snap = mixquote_to_pbquote2("", symbol, ptr, _compute_params_[symbol].depth, true);
    return true;
}

/*
bool QuoteMixer2::_on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) )
        return false;

    // 1. 需要清除的价位数据
    ptr->asks = _clear_pricelevel(exchange, ptr->asks, quote.asks, true);
    ptr->bids = _clear_pricelevel(exchange, ptr->bids, quote.bids, false);

    // 2. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, quote.asks, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, quote.bids, false);
    ptr->sequence_no = quote.sequence_no;

    // 检查发布频率
    //if( symbol == "BTC_USDT" ) {
    //    std::cout << quote.exchange << " " << quote.ask_length << " " << quote.asks[0].price.get_str_value() << " " << 
    //    ptr->asks->price.get_str_value() << " " << ptr->asks->next->price.get_str_value() << " " << ptr->asks->next->next->price.get_str_value() << std::endl;
    //}
    if( !_check_update_clocks(symbol) ) {
        return false;
    }

    std::cout << "update " << symbol << " " << ptr->ask_length() << "/" << ptr->bid_length() << std::endl;
    pub_snap = mixquote_to_pbquote3("", symbol, ptr, true);
    return true;
}
*/
bool QuoteMixer2::_get_quote(const TSymbol& symbol, SMixQuote*& ptr) const {
    auto iter = quotes_.find(symbol);
    if( iter == quotes_.end() )
        return false;
    ptr = iter->second;
    return true;
}

/*
SMixDepthPrice* QuoteMixer2::_clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const map<SDecimal, double>& newDepths, bool isAsk) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;

    for( const auto& v : newDepths ) {
        if( v.second > VOLUME_PRECISE ) {
            continue;
        }
        // 找到并删除对应价位
        while( tmp != NULL ) {
            if( tmp->price == depth.price ) {                
                unordered_map<TExchange, double>& volumeByExchange = tmp->volume;
                auto iter = volumeByExchange.find(exchange);
                if( iter != volumeByExchange.end() ) {
                    volumeByExchange.erase(iter);                
                    if( volumeByExchange.size() == 0 ) {
                        // 删除             
                        SMixDepthPrice* waitToDel = tmp;
                        last->next = tmp->next;
                        tmp = tmp->next;
                        delete waitToDel;
                        break;
                    }
                }
            } else if ( isAsk ? (tmp->price > depth.price) : (tmp->price < depth.price) ) {
                break;
            }
            last = tmp;
            tmp = tmp->next;
        }
    }
    return head.next;
}
*/

SMixDepthPrice* QuoteMixer2::_clear_exchange(const TExchange& exchange, SMixDepthPrice* depths) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;
    while( tmp != NULL ) {
        unordered_map<TExchange, double>& volumeByExchange = tmp->volume;
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

SMixDepthPrice* QuoteMixer2::_mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, double>>& depths, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;
    SMixDepthPrice* tmp = mixedDepths;

    // 1. 混合
    auto iter = depths.begin();
    for( tmp = head.next ; iter != depths.end() && tmp != NULL ; ) {
        const SDecimal& price = iter->first;
        const double& volume = iter->second;
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
        const double& volume = iter->second;
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
    int count = 0;
    while( tmp != NULL && count < MAX_MIXDEPTH) {
        tmp = tmp->next;
        count ++;
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

void process_depths(const map<SDecimal, double>& src, map<SDecimal, double>& dst, int precise, const SymbolFee& fee, bool is_ask)
{
    if( is_ask ) {
        SDecimal lastPrice = SDecimal::min_decimal();
        for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
        {
            // 卖价往上取整
            SDecimal scaledPrice;
            fee.compute(iter->first, scaledPrice, true);
            //cout << iter->first.get_str_value() << " " << precise << " " << scaledPrice.value << " " << scaledPrice.base << endl;
            scaledPrice.from(scaledPrice, precise, true); 
            //cout << iter->first.get_str_value() << " " << precise << " " << scaledPrice.value << " " << scaledPrice.base << endl;

            bool is_new_price = scaledPrice > lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice] += iter->second;
            }
        }
    } else {
        SDecimal lastPrice = SDecimal::max_decimal();
        for( auto iter = src.rbegin() ; iter != src.rend() ; iter++ ) 
        {
            // 买价往下取整
            SDecimal scaledPrice;
            fee.compute(iter->first, scaledPrice, false);
            scaledPrice.from(scaledPrice, precise, false); 

            bool is_new_price = scaledPrice < lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice] += iter->second;
            }
        }
    }
}

void QuoteMixer2::_snap_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output)
{
    std::shared_ptr<MarketStreamData> pub_snap;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        singles_[symbol][exchange] = quote;
        _precess_singles_(exchange, symbol, quote, _compute_params_[symbol].precise, _compute_params_[symbol].fees[exchange], output);
        pub_snap = depth_to_pbquote2(exchange, symbol, quote, _compute_params_[symbol].depth, true);
    }
    
    PUBLISHER->publish_single(exchange, symbol, pub_snap, NULL);
}

void QuoteMixer2::_precess_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, int precise, const SymbolFee& fee, SDepthQuote& output)
{    
    // 精度统一
    //int precise = CONFIG->get_precise(symbol);
    // 考虑手续费因素
    //SymbolFee fee = CONFIG->get_fee(exchange, symbol);

    // 直接根据quote转成output
    output.exchange = exchange;
    output.symbol = symbol;
    output.arrive_time = quote.arrive_time;
    output.sequence_no = quote.sequence_no;
    process_depths(quote.asks, output.asks, precise, fee, true);
    process_depths(quote.bids, output.bids, precise, fee, false);
}

void update_depth_diff(const map<SDecimal, double>& update, map<SDecimal, double>& dst)
{
    for( const auto& v : update ) {
        if( v.second < VOLUME_PRECISE ) {
            auto iter = dst.find(v.first);
            if( iter != dst.end() ) {
                dst.erase(iter);
            }
        } else {
            dst[v.first] = v.second;
        }
    }
}

void QuoteMixer2::_update_singles_(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& output)
{
    std::shared_ptr<MarketStreamData> pub_snap, pub_diff;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        SDepthQuote& update = singles_[symbol][exchange];
        update_depth_diff(quote.asks, update.asks);        
        update_depth_diff(quote.bids, update.bids);
        _precess_singles_(exchange, symbol, update, _compute_params_[symbol].precise, _compute_params_[symbol].fees[exchange], output);
        pub_snap = depth_to_pbquote2(exchange, symbol, update, _compute_params_[symbol].depth, true);
        pub_diff = depth_to_pbquote2(exchange, symbol, quote, _compute_params_[symbol].depth, false);
    }
    PUBLISHER->publish_single(exchange, symbol, pub_snap, pub_diff);
}

void QuoteMixer2::set_publish_params(const TSymbol& symbol, float frequency)
{
    _log_and_print("%s mix_frequency=%.03f", symbol.c_str(), frequency);

    std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
    publish_params_[symbol].frequency = frequency;
}

void QuoteMixer2::set_compute_params(const TSymbol& symbol, int precise, type_uint32 depth, const map<TExchange, SymbolFee>& fees)
{
    _log_and_print("%s precise=%d mix_depth=%u", symbol.c_str(), precise, depth);
    
    std::shared_ptr<MarketStreamData> pub_snap;

    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    // 设置参数
    _compute_params_[symbol].precise = precise;
    _compute_params_[symbol].fees = fees;
    _compute_params_[symbol].depth = depth;

    // 触发重新计算
    auto iter = singles_.find(symbol);
    if( iter != singles_.end() )
    {
        SMixQuote* ptr = quotes_[symbol];
        for( const auto& v : iter->second ) {
            const TExchange& exchange = v.first;
            const SDepthQuote& quote = v.second;
            SDepthQuote processed_quote;
            _precess_singles_(exchange, symbol, quote, _compute_params_[symbol].precise, _compute_params_[symbol].fees[exchange], processed_quote);       
            _inner_process(exchange, symbol, processed_quote, ptr);
        }
        pub_snap = mixquote_to_pbquote2("", symbol, ptr, _compute_params_[symbol].depth, true);
        inner_lock.unlock(); // 释放锁

        // 推送结果
        _publish_quote(symbol, pub_snap, NULL, true);
    }
}

/*
void QuoteMixer2::change_precise(const TSymbol& symbol, int precise)
{
    std::shared_ptr<MarketStreamData> pub_snap;

    // 清除行情数据
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        auto iter = quotes_.find(symbol);
        if( iter != quotes_.end() )
        {
            iter->second->release();
            pub_snap = mixquote_to_pbquote3("", symbol, iter->second, true);
        } else {
            SMixQuote tmp;
            pub_snap = mixquote_to_pbquote3("", symbol, &tmp, true);
        }
    }

    // 清除频率记号
    {        
        std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
        auto iter = last_clocks_.find(symbol);
        if( iter != last_clocks_.end() ) {
            last_clocks_.erase(iter);
        }
    }

    // 广播snap
    _publish_quote(symbol, pub_snap, NULL, true);
}*/