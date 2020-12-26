#include "redis_quote.h"
#include "stream_engine_config.h"

// channel_name = [channel_type]|[symbol].[exchange]
bool decode_channelname(const string& channel_name, string& channel_type, TSymbol& symbol, TExchange& exchange)
{    
    std::string::size_type pos1 = channel_name.find("|");
    if( pos1 == std::string::npos )
        return false;
    std::string::size_type pos2 = channel_name.rfind(".");
    if( pos2 == std::string::npos )
        return false;
    
    channel_type = channel_name.substr(0, pos1);
    symbol = channel_name.substr(pos1 + 1, pos2 - pos1 - 1);
    exchange = channel_name.substr(pos2 + 1);
    //cout << channel_type << " " << symbol << " " << exchange << endl;
    return true;
}

bool decode_channelmsg(const string& msg, njson& body)
{
    try
    {
        body = njson::parse(msg);
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("catch exception %s", e.what());
        return false;
    } 
    catch(...)
    {
        _log_and_print("catch unknown exception");
        return false;
    }

    return true;
}

void redisquote_to_quote_depth(const njson& data, const RedisQuote::SExchangeConfig& config, map<SDecimal, SDecimal>& depths)
{
    for (auto iter = data.begin() ; iter != data.end() ; ++iter )
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        SDecimal dPrice = SDecimal::parse(price, config.precise);        
        SDecimal dVolume = SDecimal::parse(volume, config.vprecise);
        //cout << volume << " " << config.vprecise << " " << dVolume.get_str_value() << endl;
        depths[dPrice] = dVolume;
        //cout << price << "\t" << dPrice.get_str_value() << "\t" << volume << "\t" << dVolume.get_str_value() << endl;
    }
}

bool redisquote_to_quote(const njson& snap_json, SDepthQuote& quote, const RedisQuote::SExchangeConfig& config, bool isSnap) {
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq_symbol"].get<long long>(); 
    
    quote.exchange = exchange;
    quote.symbol = symbol;
    vassign(quote.sequence_no, sequence_no);
    vassign(quote.arrive_time, parse_nano(timeArrive));
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth], config, quote.asks);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth], config, quote.bids);
    return true;
}

bool redisquote_to_kline(const njson& data, KlineData& kline, const RedisQuote::SExchangeConfig& config) 
{
    kline.index = int(data[0].get<double>());
    kline.px_open.from(data[1].get<double>(), config.precise);
    kline.px_high.from(data[2].get<double>(), config.precise);
    kline.px_low.from(data[3].get<double>(), config.precise);
    kline.px_close.from(data[4].get<double>(), config.precise);
    kline.volume.from(data[5].get<double>(), config.vprecise);
    return true;
}

bool redisquote_to_trade(const njson& data, Trade& trade, const RedisQuote::SExchangeConfig& config) 
{
    trade.time = parse_nano(data["Time"].get<string>());
    trade.price.from(data["LastPx"].get<double>(), config.precise);
    trade.volume.from(data["Qty"].get<double>(), config.vprecise);
    return true;
}

RedisQuote::~RedisQuote()
{
    if (checker_loop_) {
        if (checker_loop_->joinable()) {
            checker_loop_->join();
        }
        delete checker_loop_;
    }
}

void RedisQuote::start(const RedisParams& params, UTLogPtr logger) 
{
    // 只能调用一次
    assert( checker_loop_ == NULL );

    params_ = params;
    
    // 请求全量
    redis_snap_requester_.init(params, logger);
    redis_snap_requester_.set_engine(this);
    redis_snap_requester_.start();

    // 请求增量
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);

    // 检查连接状态
    type_tick now = get_miliseconds();
    last_time_ = now;
    checker_loop_ = new std::thread(&RedisQuote::_looping, this);
};

// return false 会重新请求快照
bool RedisQuote::_on_snap(const TExchange& exchange, const TSymbol& symbol, const string& msg) 
{    
    njson body;
    SExchangeConfig config;

    if( !decode_channelmsg(msg, body) )
    {
        _log_and_print("decode json fail %s. [exchange=%s, symbol=%s]", msg.c_str(), exchange.c_str(), symbol.c_str());
        return false;
    }
    
    if( !_get_config(exchange, symbol, config) ) 
    {
        _log_and_print("symbol not exist. [exchange=%s, symbol=%s]", exchange.c_str(), symbol.c_str());
        return false;
    }

    SDepthQuote quote;
    if( !redisquote_to_quote(body, quote, config, true))
    {
        _log_and_print("redisquote_to_quote failed. [exchange=%s, symbol=%s, msg=%s]", exchange.c_str(), symbol.c_str(), msg.c_str());
        return false;
    }
    quote.raw_length = msg.length();

    // 同步snap和updates
    list<SDepthQuote> updates_queue; // 跟在snap后面的updates
    int result = _sync_by_snap(quote.exchange, quote.symbol, quote, updates_queue);
    if( result == SYNC_STARTING )
    {
        engine_interface_->on_snap(exchange, symbol, quote);
        for( const auto& v: updates_queue ) 
        {
            engine_interface_->on_update(exchange, symbol, v);
        }
    }
    else if( result == SYNC_SNAPAGAIN ) 
    {
        return false;
    }

    return true;
};

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg)
{
    last_time_ = get_miliseconds();
    
    TExchange exchange;
    TSymbol symbol;
    njson body;
    string channel_type;
    SExchangeConfig config;

    if( !decode_channelname(channel, channel_type, symbol, exchange) )
    {
        _log_and_print("decode channel name fail. [channel=%s]", channel.c_str());
        return;
    }
    // 仅处理指定的频道
    if( channel_type != DEPTH_UPDATE_HEAD &&
        channel_type != TRADE_HEAD &&
        channel_type != KLINE_1MIN_HEAD &&
        channel_type != KLINE_60MIN_HEAD )
    {
        _log_and_print("unknown message type.[channel=%s]", channel.c_str());
        return;
    }

    if( !decode_channelmsg(msg, body) )
    {
        _log_and_print("decode json fail %s.[channel=%s]", msg.c_str(), channel.c_str());
        return;
    }
    
    if( !_get_config(exchange, symbol, config) ) 
    {
        _log_and_print("symbol not exist. [exchange=%s, symbol=%s]", exchange.c_str(), symbol.c_str());
        return;
    }
    
    if ( channel_type == DEPTH_UPDATE_HEAD )
    {
        SDepthQuote quote;
        if( !redisquote_to_quote(body, quote, config, false) ) 
        {
            _log_and_print("redisquote_to_quote failed. [channel=%s, msg=%s]", channel.c_str(), msg.c_str());
            return;
        }
        quote.raw_length = msg.length();

        // 同步snap和update
        SDepthQuote snap;
        int result = _sync_by_update(exchange, symbol, quote, snap);
        if( result == SYNC_SKIP ) 
        {
        }
        else if ( result == SYNC_STARTING )
        {
            engine_interface_->on_snap(exchange, symbol, snap);
            engine_interface_->on_update(exchange, symbol, quote);            
        } 
        else if( result == SYNC_SNAPAGAIN )
        {
            redis_snap_requester_.request_symbol(exchange, symbol);
        }
        else 
        {
            // 频率控制
            SDepthQuote tmp;
            if( !_ctrl_update(exchange, symbol, quote, config, tmp) ) 
            {
                //_log_and_print("skip %s-%s update.", exchange.c_str(), symbol.c_str());
                return;
            }
            engine_interface_->on_update(exchange, symbol, tmp);
        }
    }
    else if( channel_type == TRADE_HEAD )
    {
        Trade trade;
        if( !redisquote_to_trade(body, trade, config) )
        {
            _log_and_print("redisquote_to_trade failed. [channel=%s, msg=%s]", channel.c_str(), msg.c_str());
            return;
        }
        engine_interface_->on_trade(exchange, symbol, trade);
    }
    else if( channel_type == KLINE_1MIN_HEAD )
    {
        bool is_first_time = !kline1min_firsttime_[symbol][exchange];
        vector<KlineData> klines;
        for (auto iter = body.rbegin(); iter != body.rend(); ++iter) 
        {
            KlineData kline;
            redisquote_to_kline(*iter, kline, config);
            _log_and_print("[%s.%s] get kline1 index=%lu open=%s high=%s low=%s close=%s volume=%s", exchange.c_str(), symbol.c_str(), 
                kline.index,
                kline.px_open.get_str_value().c_str(),
                kline.px_high.get_str_value().c_str(),
                kline.px_low.get_str_value().c_str(),
                kline.px_close.get_str_value().c_str(),
                kline.volume.get_str_value().c_str()
                );
            klines.push_back(kline);
            if( is_first_time )
                break;
        }

        engine_interface_->on_kline(exchange, symbol, 60, klines, is_first_time);

        if( is_first_time )
            kline1min_firsttime_[symbol][exchange] = true;
    }
    else if( channel.find(KLINE_60MIN_HEAD) != string::npos )
    {
        bool is_first_time = !kline60min_firsttime_[symbol][exchange];
        vector<KlineData> klines;
        for (auto iter = body.rbegin(); iter != body.rend(); ++iter) 
        {
            KlineData kline;
            redisquote_to_kline(*iter, kline, config);
            _log_and_print("[%s.%s] get kline60 index=%lu open=%s high=%s low=%s close=%s volume=%s", exchange.c_str(), symbol.c_str(), 
                kline.index,
                kline.px_open.get_str_value().c_str(),
                kline.px_high.get_str_value().c_str(),
                kline.px_low.get_str_value().c_str(),
                kline.px_close.get_str_value().c_str(),
                kline.volume.get_str_value().c_str()
                );
            klines.push_back(kline);
            if( is_first_time )
                break;
        }

        engine_interface_->on_kline(exchange, symbol, 3600, klines, is_first_time);
        
        if( is_first_time )
            kline60min_firsttime_[symbol][exchange] = true;
    }
};

void RedisQuote::OnConnected() {
    _log_and_print("Redis RedisQuote::OnConnected");
};

void RedisQuote::OnDisconnected(int status) {
    _log_and_print("Redis RedisQuote::OnDisconnected");
};

int RedisQuote::_sync_by_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, list<SDepthQuote>& updates_queue)
{
    std::unique_lock<std::mutex> l{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.symbols[exchange];

    if( symbol_meta.publishing )
    {
        _log_and_print("%s.%s recv snap during publishing. SKIP.", exchange.c_str(), symbol.c_str());
        return SYNC_SKIP;
    }

    symbol_meta.snap = quote;

    // 以下尚未启动推送
    type_seqno left = symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.front().sequence_no;
    type_seqno right =  symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.back().sequence_no;
    _log_and_print("%s.%s recv snap %lu, updates from %lu to %lu.", exchange.c_str(), symbol.c_str(), quote.sequence_no, 
        symbol_meta.caches.front().sequence_no,
        symbol_meta.caches.back().sequence_no
    );

    if( right == 0 || quote.sequence_no > right ) 
    {
        symbol_meta.caches.clear();
        symbol_meta.seq_no = quote.sequence_no;
        symbol_meta.publishing = true;
        _log_and_print("%s.%s start without updates.", exchange.c_str(), symbol.c_str());
        return SYNC_STARTING;
    }
    else if( quote.sequence_no < (left - 1) ) 
    {
        _log_and_print("%s.%s request snap again.", exchange.c_str(), symbol.c_str());
        return SYNC_SNAPAGAIN;
    }
    else // left <= quote.sequence_no <= right
    {        
        assert( left <= quote.sequence_no && quote.sequence_no <= right );

        for( const auto& v : symbol_meta.caches ) 
        {
            if( v.sequence_no >= (quote.sequence_no + 1 ) ) 
            {
                updates_queue.push_back(v);
            }
        }
        symbol_meta.caches.clear();
        symbol_meta.seq_no = right;
        symbol_meta.publishing = true;
        _log_and_print("%s.%s start with %lu updates.", exchange.c_str(), symbol.c_str(), updates_queue.size());
        return SYNC_STARTING;
    }

    // never reach here
    assert(false);
    return SYNC_OK;
}

int RedisQuote::_sync_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& snap)
{
    std::unique_lock<std::mutex> l{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.symbols[exchange];

    // 更新统计信息
    meta.pkg_count += 1;
    meta.pkg_size += quote.raw_length;

    // 序号不连续
    if( quote.sequence_no != (symbol_meta.seq_no + 1) ) 
    {
        _log_and_print("%s.%s sequence skip from %lu to %lu. refresh snap again.", 
                        exchange.c_str(), symbol.c_str(), symbol_meta.seq_no, quote.sequence_no);
        redis_snap_requester_.request_symbol(exchange, symbol);
        symbol_meta.caches.clear();
        symbol_meta.caches.push_back(quote);
        symbol_meta.publishing = false;
        symbol_meta.seq_no = quote.sequence_no;

        meta.pkg_skip_count += 1;

        return SYNC_SNAPAGAIN;
    }

    symbol_meta.seq_no = quote.sequence_no;

    if( symbol_meta.publishing )
    {
        return SYNC_OK;
    }

    // 尚未启动推送
    if( quote.sequence_no > symbol_meta.snap.sequence_no )
    {
        symbol_meta.caches.push_back(quote);
    }
    return SYNC_SKIP;
};

void RedisQuote::_looping() 
{
    last_statistic_time_ = get_miliseconds();
    last_nodata_time_ = get_miliseconds();

    while( true ) 
    {
        // 休眠
        std::this_thread::sleep_for(std::chrono::microseconds(10));

        type_tick now = get_miliseconds();

        // 强制刷出没有后续更新的品种
        /*
        vector<SDepthQuote> forced_to_update;
        if( false ) {
            std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
            for( auto& v : last_clocks_ )
            {
                const TSymbol& symbol = v.first;
                for( auto& u : v.second )
                {                    
                    const TExchange& exchange = u.first;
                    float frequency = frequecy_[symbol];
                    if( frequency != 0 && (now - u.second) < (1000/frequency) ) {
                        continue;
                    }
                    u.second = now;
                    
                    SDepthQuote& quote = updates_[symbol][exchange];
                    SDepthQuote tmp;
                    tmp.exchange = exchange;
                    tmp.symbol = symbol;
                    tmp.arrive_time = quote.arrive_time;
                    tmp.asks.swap(quote.asks);
                    tmp.bids.swap(quote.bids);
                    if( tmp.asks.size() == 0 && tmp.bids.size() == 0 ) {
                        continue;
                    }
                    forced_to_update.push_back(tmp);
                }
            }
        }
        for( const auto& v : forced_to_update ) {            
            engine_interface_->on_update(v.exchange, v.symbol, v);
        }
        */

        // 检查心跳
        if( now > last_time_ && (now - last_time_) > 10 * 1000 ) {
            _log_and_print("heartbeat expired. [now=%ld, last=%ld]", now, last_time_);
            _loopng_check_heartbeat();
        }

        // 检查异常没有数据的交易所
        if( (now - last_nodata_time_) > 30*1000 ) 
        {
            _looping_checknodata();
            last_nodata_time_ = now;
        }

        // 打印统计信息
        if( (now - last_statistic_time_) > 10*1000 ) 
        {
            _looping_print_statistics();
            last_statistic_time_ = now;
        }
    }
}

void RedisQuote::_loopng_check_heartbeat()
{
    // 请求增量
    _log_and_print("reconnect redis ...");
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);
    last_time_ = get_miliseconds();
    last_redis_time_ = get_miliseconds();
    _log_and_print("reconnect redis ok.");

}

void RedisQuote::_looping_checknodata()
{
    unordered_set<TExchange> nodata_exchanges;
    {
        std::unique_lock<std::mutex> l{ mutex_metas_ };
        for( const auto& v : metas_ ) {
            if( v.second.pkg_count == 0 ) {
                nodata_exchanges.insert(v.first);
                _log_and_print("exchange %s no data", v.first.c_str());
            }
        }

        for( const auto& v : nodata_exchanges ) {
            metas_.erase(v);
        }
    }

    // 通知下游模块删除交易所
    for( const auto& v : nodata_exchanges ) {
        engine_interface_->on_nodata_exchange(v);
    }
}

void RedisQuote::_looping_print_statistics()
{
    unordered_map<TExchange, ExchangeMeta> statistics;
    {
        std::unique_lock<std::mutex> l{ mutex_metas_ };
        statistics = metas_;
        for( auto& v : metas_ ) {
            v.second.reset();
        }
    }

    _println_("-------------");
    ExchangeMeta total;
    for( const auto& v : statistics ) {
        _println_("%s\t\t%s", v.first.c_str(), v.second.get().c_str());
        total.accumlate(v.second);
    }
    _println_("total\t\t%s", total.get().c_str());
    _println_("-------------");

}

bool RedisQuote::_ctrl_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, const SExchangeConfig& config, SDepthQuote& update) 
{
    std::unique_lock<std::mutex> l{ mutex_clocks_ };

    // 没有频率控制，直接更新
    if( config.frequency == 0 )
    {
        update = quote;
        return true;
    }

    _SFrequencyMeta& meta = frequency_metas_[symbol];

    // 保留更新
    SDepthQuote& cache = meta.updates[exchange];
    cache.arrive_time = quote.arrive_time;
    for( const auto& v : quote.asks )
    {
        cache.asks[v.first] = v.second;
    }
    for( const auto& v : quote.bids )
    {
        cache.bids[v.first] = v.second;
    }

    // 频率控制
    auto last = meta.last_clocks[exchange];
    auto now = get_miliseconds();
    if( (now - last) < (1000/config.frequency) ) {
        return false;
    }

    meta.last_clocks[exchange] = now;
    // 生成新的增量
    update.exchange = exchange;
    update.symbol = symbol;
    update.arrive_time = cache.arrive_time;
    update.asks.swap(cache.asks);
    update.bids.swap(cache.bids);
    return true;
}

void RedisQuote::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    // 打印入参
    string desc = "";
    for( const auto& v : config ) {
        desc += v.first + ":" + ToString(v.second.precise) + ":" + ToString(v.second.vprecise);
    }
    _log_and_print("%s exchanges=%s", symbol.c_str(), desc.c_str());

    // 设置交易所配置
    {
        std::unique_lock<std::mutex> l{ mutex_symbol_ };
        // 删除老的
        const auto& iter = symbols_.find(symbol);
        if( iter != symbols_.end() )
        {
            for( const auto& v : iter->second ) {
                unsubscribe(v.first, symbol);
            }
        }

        // 添加新的    
        for( const auto& v : config )
        {
            subscribe(v.first, symbol);
        }

        // 赋值
        symbols_[symbol] = config;
    }
}

bool RedisQuote::_get_config(const TExchange& exchange, const TSymbol& symbol, SExchangeConfig& config) const 
{
    std::unique_lock<std::mutex> l{ mutex_symbol_ };
    auto v = symbols_.find(symbol);
    if( v == symbols_.end() )
        return false;
    const SSymbolConfig& configs = v->second;        
    auto v2 = configs.find(exchange);
    if( v2 == configs.end() )
        return false;
    config = v2->second;
    return true;
}