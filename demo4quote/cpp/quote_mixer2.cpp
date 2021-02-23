#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "converter.h"

bool QuoteCacher::get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps, const TExchange* fix_exchange)
{
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    for( const auto& v : singles_ ) {
        const TSymbol& symbol = v.first;
        for( const auto& v2 : v.second ) {
            const TExchange& exchange = v2.first;
            if( fix_exchange != NULL && *fix_exchange != exchange )
                continue;
            std::shared_ptr<MarketStreamDataWithDecimal> snap = depth_to_pbquote2(exchange, symbol, v2.second, publish_depths_, true);
            snaps.push_back(snap);
        }
    }
    return true;
}

bool QuoteCacher::get_latetrades(vector<std::shared_ptr<TradeWithDecimal>>& trades)
{
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    for( const auto& s : trades_ ) {
        for( const auto & e : s.second ) {
            const TSymbol& symbol = s.first;
            const TExchange& exchange = e.first;
            const Trade& trade = e.second;
            std::shared_ptr<TradeWithDecimal> tmp = trade_to_pbtrade(exchange, symbol, trade);
            trades.push_back(tmp);
        }
    }
    return true;
}

void QuoteCacher::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        trades_[symbol][exchange] = trade;
    }

    std::shared_ptr<TradeWithDecimal> pub_trade = trade_to_pbtrade(exchange, symbol, trade);
    for( const auto& v : callbacks_) 
    {
        v->publish_trade(exchange, symbol, pub_trade);
    }
}

void QuoteCacher::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        singles_[symbol][exchange] = quote;
    }
    
    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap = depth_to_pbquote2(exchange, symbol, quote, publish_depths_, true);
    for( const auto& v : callbacks_) 
    {
        v->publish_binary(exchange, symbol, pub_snap);
    }
}

void QuoteCacher::clear_exchange(const TExchange& exchange)
{    
    unordered_map<TSymbol, SDepthQuote> snaps;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( auto&v : singles_ ) 
        {
            auto iter = v.second.find(exchange);
            if( iter == v.second.end() )
                continue;
            SDepthQuote empty;
            empty.exchange = exchange;
            empty.symbol = v.first;
            v.second.erase(iter);
            snaps[v.first] = empty;
        }
    }

    for( const auto& snap : snaps )
    {
        const TSymbol& symbol = snap.first;
        const SDepthQuote& quote = snap.second;
        for( const auto& v : callbacks_) 
        {
            std::shared_ptr<MarketStreamDataWithDecimal> pub_snap = depth_to_pbquote2(exchange, symbol, quote, publish_depths_, true);
            v->publish_binary(exchange, symbol, pub_snap);
        }
    }
}

void QuoteCacher::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& update, SDepthQuote& snap) 
{
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        SDepthQuote& cache = singles_[symbol][exchange];
        update_depth_diff(update.asks, cache.asks);        
        update_depth_diff(update.bids, cache.bids);
        snap = cache;
    }

    std::shared_ptr<MarketStreamDataWithDecimal> pub_diff;
    pub_diff = depth_to_pbquote2(exchange, symbol, update, publish_depths_*2, false); // 增量的档位应该是2倍
    for( const auto& v : callbacks_) 
    {
        v->publish_binary(exchange, symbol, pub_diff);
    }
}

/////////////////////////////////////////////////////////////////////
QuoteMixer2::QuoteMixer2()
: thread_run_(true)
{

}

QuoteMixer2::~QuoteMixer2()
{
    thread_run_ = false;
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}


void QuoteMixer2::start()
{
    if( thread_loop_ )
        return;
    thread_loop_ = new std::thread(&QuoteMixer2::_thread_loop, this);
}

void QuoteMixer2::_thread_loop()
{
    unordered_map<TSymbol, type_seqno> sequences;

    while( thread_run_ ) 
    {
        vector<pair<TSymbol, SMixerConfig>> calculate_symbols;
        {
            type_tick now = get_miliseconds();
            std::unique_lock<std::mutex> l{ mutex_config_ };
            for( const auto& cfg : configs_ )
            {      
                const TSymbol& symbol = cfg.first;    
                float frequency = cfg.second.frequency;
                if( frequency == 0 )
                    frequency = 1;
                auto iter = last_clocks_.find(symbol);                
                if( iter != last_clocks_.end() && (now - iter->second) < (1000/frequency) )
                {
                    continue;
                }
                calculate_symbols.push_back(make_pair(symbol, cfg.second));
                last_clocks_[symbol] = now;
            }
        }
        
        // 逐个计算
        for( const auto& symbol : calculate_symbols ) {
            sequences[symbol.first] ++;
            _calc_symbol(symbol.first, symbol.second, sequences[symbol.first]);
        }
        
        // 休眠
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void mix_quote(map<SDecimal, SDepth>& dst, const map<SDecimal, SDepth>& src, const TExchange& exchange, const QuoteMixer2::SMixerConfig& config, bool is_ask)
{    
    SymbolFee fee;
    auto iter = config.fees.find(exchange);
    if( iter != config.fees.end() ) {
        fee = iter->second;
    }

    for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
    {
        SDecimal scaledPrice;
        fee.compute(iter->first, scaledPrice, is_ask);
        scaledPrice.scale(config.precise, is_ask);
        dst[scaledPrice].volume_by_exchanges[exchange] += iter->second.volume;
    }
}

void normalize(map<SDecimal, SDepth>& src, const QuoteMixer2::SMixerConfig& config)
{
    SDecimal volume = 0;
    for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
    {
        volume = 0;
        for( auto& v : iter->second.volume_by_exchanges ) {
            v.second.scale(config.vprecise);
            volume += v.second;
        }
        iter->second.volume = volume;
    }
}

void QuoteMixer2::_calc_symbol(const TSymbol& symbol, const SMixerConfig& config, type_seqno seqno)
{
    // 行情
    //cout << "calulate " << symbol << endl;
    SDepthQuote snap;
    snap.sequence_no = seqno;
    snap.origin_time = snap.arrive_time = get_miliseconds();
    snap.price_precise = config.precise;
    snap.volume_precise = config.vprecise;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( const auto& data : quotes_[symbol] ) 
        {
            const TExchange& exchange = data.first;
            const SDepthQuote& quote = data.second;
            mix_quote(snap.asks, quote.asks, exchange, config, true);
            mix_quote(snap.bids, quote.bids, exchange, config, false);
        }
    }
    normalize(snap.asks, config);
    normalize(snap.bids, config);

    if( snap.origin_time > 0 ) {
        if( (get_miliseconds() / 1000 % 10) == 0 ) { // 每10秒输出一次
            _log_and_print("publish %s.%s %u/%u", MIX_EXCHANGE_NAME, symbol, snap.asks.size(), snap.bids.size());
        }
        engine_interface_->on_snap(MIX_EXCHANGE_NAME, symbol, snap);
    }

    // 交易
    Trade trade;
    {        
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( const auto& data : trades_[symbol] ) 
        {
            if( data.second.time > trade.time )
                trade = data.second;
        }
    }
    trade.price.scale(config.precise);
    trade.volume.scale(config.vprecise);
    if( trade.time > 0 ) {
        engine_interface_->on_trade(MIX_EXCHANGE_NAME, symbol, trade);
    }
}

void QuoteMixer2::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    trades_[symbol][exchange] = trade;
}

void QuoteMixer2::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    quotes_[symbol][exchange] = quote;
}

/*

void process_depths(const map<SDecimal, SDepth>& src, map<SDecimal, SDepth>& dst, int precise, int vprecise, const SymbolFee& fee, bool is_ask)
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
            //cout << iter->first.get_str_value() << " " << precise << " " << scaledPrice.value << " " << scaledPrice.base << endl;

            bool is_new_price = scaledPrice > lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice].volume = dst[lastPrice].volume + iter->second.volume;
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

            bool is_new_price = scaledPrice < lastPrice;
            if( is_new_price ) {
                dst[scaledPrice] = iter->second;
                lastPrice = scaledPrice;
            } else {
                dst[lastPrice].volume = dst[lastPrice].volume + iter->second.volume;
            }
        }
    }

    // 缩放成交量
    for( auto& v : dst ) 
    {
        v.second.volume.scale(vprecise, false);
    }
}

bool QuoteMixer2::_check_clocks(const TSymbol& symbol, float frequency, unordered_map<TSymbol, type_tick>& clocks) 
{
    if( frequency == 0 )
        return true;

    std::unique_lock<std::mutex> l{ mutex_clocks_ };
    // 每秒更新频率控制
    auto iter = clocks.find(symbol);
    if( iter != clocks.end() && (get_miliseconds() -iter->second) < (1000/frequency) )
    {
        return false;
    }
    clocks[symbol] = get_miliseconds();
    return true;
}

void QuoteMixer2::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    SSymbolConfig config;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        config = configs_[symbol];
    }

    if( !_check_clocks(symbol, config.frequency, last_trade_clocks_) ) {
        return;
    }

    engine_interface_->on_trade("", symbol, trade);
}

void QuoteMixer2::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    SSymbolConfig config;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        config = configs_[symbol];
    }

    // 叠加交易所手续费
    SDepthQuote output;
    process_depths(quote.asks, output.asks, config.precise, config.vprecise, config.fees[exchange], true);
    process_depths(quote.bids, output.bids, config.precise, config.vprecise, config.fees[exchange], false);

    // 合并数据
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    SDepthQuote& snap = quotes_[symbol];
    snap.symbol = symbol;
    snap.arrive_time = snap.origin_time = snap.server_time = get_miliseconds();
    snap.price_precise = config.precise;
    snap.volume_precise = config.vprecise;

    // 频率控制
    if( !_check_clocks(symbol, config.frequency, last_snap_clocks_) ) {
        return;
    }

    std::cout << "publish(snap) " << symbol << " " << snap.asks.size() << "/" << snap.bids.size() << std::endl;
    for( const auto& v : callbacks_) 
    {
        v->publish_mix(symbol, snap);
    }

    engine_interface_->on_snap("", symbol, snap);

    std::shared_ptr<MarketStreamDataWithDecimal> pub_snap;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) ) {
            ptr = new SMixQuote();
            quotes_[symbol] = ptr;
        }
        
        _inner_process(exchange, symbol, output, ptr);

        if( !_check_clocks(symbol, config.frequency, last_snap_clocks_) ) {
            return;
        }

        pub_snap = mixquote_to_pbquote2("", symbol, ptr, config.depth, true);
    }
    
    std::cout << "publish(snap) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    for( const auto& v : callbacks_) 
    {
        v->publish_mix(symbol, pub_snap);
    }
}

void QuoteMixer2::_inner_process(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SMixQuote* ptr)
{
    ptr->server_time = get_miliseconds();
    ptr->price_precise = quote.price_precise;
    ptr->volume_precise = quote.volume_precise;
    
    // 1. 清除老的exchange数据
    ptr->asks = _clear_exchange(exchange, ptr->asks);
    ptr->bids = _clear_exchange(exchange, ptr->bids);

    // 2. 合并价位
    vector<pair<SDecimal, SDepth>> depths;
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

bool QuoteMixer2::get_lastsnaps(vector<std::shared_ptr<MarketStreamDataWithDecimal>>& snaps)
{    
    std::unique_lock<std::mutex> l{ mutex_quotes_ };
    for( const auto & v : quotes_ )
    {
        SSymbolConfig config = configs_[v.first];
        std::shared_ptr<MarketStreamDataWithDecimal> pub_snap = mixquote_to_pbquote2("", v.first, v.second, config.depth, true);
        snaps.push_back(pub_snap);
    }
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

SMixDepthPrice* QuoteMixer2::_mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const vector<pair<SDecimal, SDepth>>& depths, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;
    SMixDepthPrice* tmp = mixedDepths;

    // 1. 混合
    auto iter = depths.begin();
    for( tmp = head.next ; iter != depths.end() && tmp != NULL ; ) {
        const SDecimal& price = iter->first;
        const SDecimal& volume = iter->second.volume;
        if( volume.is_zero() ) {
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
        const SDecimal& volume = iter->second.volume;
        if( volume.is_zero() ) {
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
*/

void QuoteMixer2::set_config(const TSymbol& symbol, const SMixerConfig& config)
{
    _log_and_print("QuoteMixer2::set_config %s", symbol);
    std::unique_lock<std::mutex> l{ mutex_config_ };
    configs_[symbol] = config;
}

