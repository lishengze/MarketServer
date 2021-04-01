#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "converter.h"

void set_static_variable(const TExchange& exchange, const TSymbol& symbol, const type_tick& origin_time)
{
    if( string(exchange) == "HUOBI" && string(symbol) == "BTC_USDT" ) {
        CONFIG->GLOBAL_HUOBI_BTC = origin_time/1000000000;
    } else if( string(exchange) == "BINANCE" && string(symbol) == "BTC_USDT" ) {
        CONFIG->GLOBAL_BINANCE_BTC = origin_time/1000000000;
    } else if( string(exchange) == "OKEX" && string(symbol) == "BTC_USDT" ) {
        CONFIG->GLOBAL_OKEX_BTC = origin_time/1000000000;
    } else if( string(exchange) == MIX_EXCHANGE_NAME && string(symbol) == "BTC_USDT" ) {
        CONFIG->GLOBAL_BCTS_BTC = origin_time/1000000000;
    } else {

    }
}
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

void QuoteCacher::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    set_static_variable(exchange, symbol, quote.origin_time);

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

void QuoteCacher::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& update, SDepthQuote& snap) 
{
    set_static_variable(exchange, symbol, update.origin_time);

    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        SDepthQuote& cache = singles_[symbol][exchange];
        cache.origin_time = update.origin_time;
        cache.arrive_time = update.arrive_time;
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
    snap.origin_time = 0;
    snap.arrive_time = get_miliseconds();
    snap.price_precise = config.precise;
    snap.volume_precise = config.vprecise;
    snap.amount_precise = config.aprecise;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( const auto& data : quotes_[symbol] ) 
        {
            const TExchange& exchange = data.first;
            const SDepthQuote& quote = data.second;
            if( quote.origin_time > snap.origin_time ) // 交易所时间取聚合品种中较大的
                snap.origin_time = quote.origin_time;
            mix_quote(snap.asks, quote.asks, exchange, config, true);
            mix_quote(snap.bids, quote.bids, exchange, config, false);
        }
    }
    normalize(snap.asks, config);
    normalize(snap.bids, config);

    if( snap.origin_time > 0 ) {
        //if( (get_miliseconds() / 1000 % 10) == 0 ) { // 每10秒输出一次
        //    _log_and_print("publish %s.%s %u/%u", MIX_EXCHANGE_NAME, symbol, snap.asks.size(), snap.bids.size());
        //}
        // tfm::printfln("%s %lu", symbol, snap.origin_time);
        // if (symbol == "BTC_USDT")
        //     std::cout << MIX_EXCHANGE_NAME << " " << symbol << " " << snap.asks.size() << "/" << snap.bids.size() << std::endl;
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

void QuoteMixer2::set_config(const TSymbol& symbol, const SMixerConfig& config)
{
    _log_and_print("QuoteMixer2::set_config %s", symbol);
    std::unique_lock<std::mutex> l{ mutex_config_ };
    configs_[symbol] = config;
}

