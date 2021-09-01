#include "redis_quote.h"
#include "stream_engine_config.h"
#include "converter.h"
// json库
#include "base/cpp/rapidjson/document.h"
#include "base/cpp/rapidjson/writer.h"
#include "base/cpp/rapidjson/stringbuffer.h"

#include "pandora/util/time_util.h"

#include "Log/log.h"
#include "util/tool.h"

using namespace rapidjson;

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

bool decode_channelmsg(const string& msg, Document& body)
{
    body.Parse(msg.c_str());
    if(body.HasParseError())
    {
        return false;
    }
    return true;
}

void redisquote_to_quote_depth(const Value& data, const SExchangeConfig& config, map<SDecimal, SDepth>& depths)
{
    for (auto iter = data.MemberBegin() ; iter != data.MemberEnd() ; ++iter )
    {
        const string& price = iter->name.GetString();
        const double& volume = iter->value.GetDouble();
        SDecimal dPrice = SDecimal::parse(price, config.precise);
        SDecimal dVolume = SDecimal::parse(volume, config.vprecise);
        depths[dPrice].volume = dVolume;
    }
}

bool redisquote_to_quote(const Document& snap_json, SDepthQuote& quote, const SExchangeConfig& config, bool isSnap) {
    quote.asks.clear();
    quote.bids.clear();
    
    // 使用channel里面的交易所和代码，不使用json中的
    //string symbol = snap_json["Symbol"].get<std::string>();
    //string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].GetString();
    type_seqno sequence_no = snap_json["Msg_seq_symbol"].GetUint64(); 
    
    quote.price_precise = config.precise;
    quote.volume_precise = config.vprecise;
    quote.amount_precise = config.aprecise;
    vassign(quote.sequence_no, sequence_no);
    vassign(quote.origin_time, parse_nano(timeArrive));
    quote.arrive_time = get_miliseconds();
    quote.server_time = 0; // 这个时间应该在发送前赋值
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth.c_str()], config, quote.asks);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth.c_str()], config, quote.bids);
    return true;
}

bool redisquote_to_kline(const Value& data, KlineData& kline, const SExchangeConfig& config) 
{
    kline.index = int(data[0].GetDouble());
    kline.px_open.from(data[1].GetDouble(), config.precise);
    kline.px_high.from(data[2].GetDouble(), config.precise);
    kline.px_low.from(data[3].GetDouble(), config.precise);
    kline.px_close.from(data[4].GetDouble(), config.precise);
    kline.volume.from(data[5].GetDouble(), config.vprecise);
    return true;
}

bool redisquote_to_trade(const Document& data, Trade& trade, const SExchangeConfig& config) 
{
    trade.time = parse_nano(data["Time"].GetString());  // 2020-12-27 12:48:41.578000
    trade.price.from(data["LastPx"].GetDouble(), config.precise);
    trade.volume.from(data["Qty"].GetDouble(), config.vprecise);
    return true;
}

bool valida_kline(const KlineData& kline) {
    return !(kline.index < 1000000000 || kline.index > 1900000000);
}

bool on_get_message(RedisKlineHelper& helper, const Document& body, const TExchange& exchange, const TSymbol& symbol, const SExchangeConfig& config, vector<KlineData>& klines)
{
    type_tick last_index = helper.first_package_[symbol][exchange];
    for (auto iter = body.Begin(); iter != body.End(); ++iter) 
    {
        KlineData kline;
        redisquote_to_kline(*iter, kline, config);
        if( !valida_kline(kline) ) {
            LOG_WARN("[kline min] get abnormal kline data " + std::to_string(helper.resolution_));
            continue;
        }
        if( kline.index < last_index ) {
            continue;
        }
        klines.push_back(kline);
    }

    if( klines.size() > 0 )
        helper.first_package_[symbol][exchange] = klines.back().index;

    return last_index == 0;
}

RedisQuote::RedisQuote()
: thread_run_(true)
, kline_min1_(60)
, kline_min60_(3600)
{}

RedisQuote::~RedisQuote()
{
    thread_run_ = false;
    if (checker_loop_) {
        if (checker_loop_->joinable()) {
            checker_loop_->join();
        }
        delete checker_loop_;
    }
}

void RedisQuote::init(QuoteSourceCallbackInterface* callback, const RedisParams& params, UTLogPtr logger, bool dump)
{    
    LOG_INFO("Redis Connnect " + params.host + ":" + std::to_string(params.port) + ", pwd: " + params.password);

    dump_ = dump;

    engine_interface_ = callback;

    // 请求全量
    redis_snap_requester_.init(params, logger, this);

    // 请求增量
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(params.host, params.port, params.password, utrade::pandora::RM_Subscribe);
}

void RedisQuote::init_replay(QuoteSourceCallbackInterface* callback, int ratio, int replicas)
{
    replay_ = true;

    engine_interface_ = callback;

    quote_replayer_.init(this, ratio, replicas);
}

bool RedisQuote::start() 
{    
    // 启动录数据线程
    if( dump_ ) {
       quote_dumper_.start();
    }

    // 回放数据
    if( replay_ ) {
        quote_replayer_.start();
    } else {
        redis_snap_requester_.start();

        if( checker_loop_ )
            return false;

        type_tick now = get_miliseconds();
        last_time_ = now;
        checker_loop_ = new std::thread(&RedisQuote::_looping, this);
    }
    
    return true;
};

bool RedisQuote::stop()
{
    return true;
};

void RedisQuote::on_message(const std::string& channel, const std::string& msg, bool& retry)
{
    // 录数据
    if( dump_ ) {
       quote_dumper_.add_message(channel, msg);
    }

    last_time_ = get_miliseconds();
    
    TExchange exchange;
    TSymbol symbol;
    //njson body;
    Document body;
    string channel_type;
    SExchangeConfig config;

    LOG->record_input_info(channel);

    if( !decode_channelname(channel, channel_type, symbol, exchange) )
    {
        LOG_WARN("decode channel name fail. channel="+channel);
        
        return;
    }
    // 仅处理指定的频道
    if( channel_type != DEPTH_UPDATE_HEAD &&
        channel_type != TRADE_HEAD &&
        channel_type != KLINE_1MIN_HEAD &&
        channel_type != KLINE_60MIN_HEAD &&
        channel_type != SNAP_HEAD )
    {
        LOG_WARN("unknown message type. channel=" + channel);
        return;
    }

    if( !decode_channelmsg(msg, body) )
    {
        LOG_WARN("decode json fail\nmsg: " + msg + "\nchannel: " + channel);
        return;
    }

    // if (symbol == "ETH_BTC" && channel_type == SNAP_HEAD)
    // {
    //     StringBuffer buffer;
    //     Writer<StringBuffer> writer(buffer);
    //     body.Accept(writer);

    //     LOG_DEBUG("\nJson Parse Data: \n" + buffer.GetString());
    // }
   
    if( !_get_config(exchange, symbol, config) ) 
    {
        LOG_WARN("symbol not exist. exchange=" + exchange +" symbol=" + symbol);        
        return;
    }

    if( !config.enable )
    {
        LOG_WARN("Not able exchange=" + exchange +" symbol=" + symbol);     
        return;
    }
        
    
    if( channel_type == SNAP_HEAD )
    {
        // 设置默认不重试
        retry = false;

        SDepthQuote quote;
        quote.exchange = exchange;
        quote.symbol = symbol;
        if( !redisquote_to_quote(body, quote, config, true))
        {
            LOG_WARN("redisquote_to_quote failed. channel=" + channel + ", msg=" + msg);            
            retry = true;
            return;
        }
        quote.raw_length = msg.length();
        // 同步snap和updates
        list<SDepthQuote> updates_queue; // 跟在snap后面的updates

        int result = _sync_by_snap(quote.exchange, quote.symbol, quote, updates_queue);

        // if( result == SYNC_STARTING )
        // {
        //     engine_interface_->on_snap(exchange, symbol, quote);
        //     // 合并更新
        //     SDepthQuote fake_update = quote;
        //     fake_update.asks.clear();
        //     fake_update.bids.clear();
        //     for( const auto& v: updates_queue ) 
        //     {
        //         fake_update.arrive_time = v.arrive_time;
        //         fake_update.server_time = v.server_time;
        //         fake_update.sequence_no = v.sequence_no;
        //         update_depth_diff(v.asks, fake_update.asks);
        //         update_depth_diff(v.bids, fake_update.bids);
        //     }
            // 合并更新
            // SDepthQuote merged_update = merge_update(updates_queue, quote);
            // engine_interface_->on_update(exchange, symbol, merged_update);        
        //     engine_interface_->on_update(exchange, symbol, fake_update);
        // }

        if( result == SYNC_STARTING )
        {
            if (filter_zero_volume(quote))
            {
                LOG_WARN("\nSnap Quote: \n" + quote_str(quote));
            }
            else
            {
                // if (quote.symbol == "ETH_BTC")
                // {
                //     LOG_DEBUG("\nAfter _sync_by_snap Snap: \n" + quote_str(quote));
                // }                
                engine_interface_->on_snap(exchange, symbol, quote);
            }            
        }
        else if( result == SYNC_REQ_SNAP ) 
        {
            retry = true;
            redis_snap_requester_.async_request_symbol(exchange, symbol);
            return;
        }
    }
    else if ( channel_type == DEPTH_UPDATE_HEAD )
    {
        SDepthQuote quote;
        quote.exchange = exchange;
        quote.symbol = symbol;
        if( !redisquote_to_quote(body, quote, config, false) ) 
        {
            LOG_WARN("redisquote_to_quote failed. channel=" + channel + ", msg=" + msg);       
            return;
        }
        quote.raw_length = msg.length();

        // if (quote.symbol == "ETH_USDT")
        // {
        //     LOG_DEBUG("\nOrigianl Update: " + quote_str(quote));
        // }


        // 同步snap和update
        SDepthQuote snap;
        int result = _sync_by_update(exchange, symbol, quote, snap);
        if( result == SYNC_SKIP ) 
        {
            // 继续等待 由于乱序 发生，从而异步请求的 snap 的到来;
        }
        // else if ( result == SYNC_STARTING ) // 不会发生；
        // {
        //     engine_interface_->on_snap(exchange, symbol, snap);
        //     engine_interface_->on_update(exchange, symbol, quote);            
        // } 
        else if( result == SYNC_REQ_SNAP ) // 发生乱序，请求 snap;
        {
            redis_snap_requester_.async_request_symbol(exchange, symbol);
        }
        else if (result == SYNC_OK)
        {
            // 频率控制
            SDepthQuote tmp;
            if( !_ctrl_update(exchange, symbol, quote, config, tmp) ) 
            {
                LOG_WARN("skip update." + exchange + "." + symbol);
                return;
            }

            if (filter_zero_volume(tmp))
            {
                LOG_WARN("\nUpdate Quote : \n" + quote_str(tmp));
            }
            else
            {
                engine_interface_->on_update(exchange, symbol, tmp);
            }            
        }
        else
        {
            LOG_ERROR("Unknow SYNC_TYPE: " + std::to_string(result));
        }
    }
    else if( channel_type == TRADE_HEAD )
    {
        Trade trade;
        if( !redisquote_to_trade(body, trade, config) )
        {
            LOG_WARN("redisquote_to_quote failed. channel=" + channel + ", msg=" + msg);       
            return;
        }
        engine_interface_->on_trade(exchange, symbol, trade);
    }
    else if( channel_type == KLINE_1MIN_HEAD )
    {
        vector<KlineData> klines;
        bool first_package = on_get_message(kline_min1_, body, exchange, symbol, config, klines);

        // if (symbol == "USDT_USD")
        // {
        //     LOG_DEBUG("\nOrigianl Kline: " + klines_str(klines));
        // }

        filter_kline_data(klines);

        engine_interface_->on_kline(exchange, symbol, kline_min1_.resolution_, klines, first_package);
    }
    else if( channel.find(KLINE_60MIN_HEAD) != string::npos )
    {
        vector<KlineData> klines;
        bool first_package = on_get_message(kline_min60_, body, exchange, symbol, config, klines);
        engine_interface_->on_kline(exchange, symbol, kline_min60_.resolution_, klines, first_package);
    }
};

void RedisQuote::OnConnected() 
{
    connected_ = true;
    LOG_INFO("Redis RedisQuote::OnConnected");

    // 设置交易所配置
    std::unique_lock<std::mutex> l{ mutex_symbol_ };
    for( const auto& v : symbols_ ) {
        for( const auto& v2 : v.second ) {
            LOG_INFO("subscribe " + v2.first + ", " + v.first);

            subscribe(v2.first, v.first);
        }
    }
};

void RedisQuote::OnDisconnected(int status) {
    connected_ = false;
    LOG_WARN("Redis RedisQuote::OnDisconnected");
};

int RedisQuote::_sync_by_snap(const TExchange& exchange, const TSymbol& symbol, SDepthQuote& quote, list<SDepthQuote>& updates_queue)
{
    std::unique_lock<std::mutex> l{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.exchanges[exchange];

    // 已经启动推送
    if( symbol_meta.publishing )
    {
        LOG_WARN("Skip publish recv snap " + exchange + "." +symbol);        
        return SYNC_SKIP;
    }

    symbol_meta.snap = quote;

    // 以下尚未启动推送
    type_seqno left = symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.front().sequence_no;
    type_seqno right =  symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.back().sequence_no;

    if( right == 0 || quote.sequence_no > right ) 
    {
        symbol_meta.caches.clear();
        symbol_meta.seq_no = quote.sequence_no;
        symbol_meta.publishing = true;
        return SYNC_STARTING;
    }
    else if( quote.sequence_no < (left - 1) ) 
    {
        LOG_WARN("Snap.Seq: " + std::to_string(quote.sequence_no) + ", Cached.LeftSeq: " + std::to_string(left));
        return SYNC_REQ_SNAP;
    }
    else if (left <= quote.sequence_no && quote.sequence_no <= right )
    {
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

        // 直接合并 Snap 和 CachedQuote;
        {
            updates_queue.push_front(quote);
            quote = merge_quote(updates_queue);
            quote.exchange = exchange;
            quote.symbol = symbol;
        }

        return SYNC_STARTING;
    }
    else
    {
        LOG_ERROR("Snap.Seq Error " + std::to_string(quote.sequence_no) 
                    + ", Cached.LeftSeq: " + std::to_string(left)
                    + ", Cached.RightSeq: " + std::to_string(right));
        return SYNC_REQ_SNAP;
    }

    // never reach here
    assert(false);
    return SYNC_OK;
}

int RedisQuote::_sync_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, SDepthQuote& snap)
{
    std::unique_lock<std::mutex> l{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.exchanges[exchange];

    // 更新统计信息
    meta.pkg_count += 1;
    meta.pkg_size += quote.raw_length;

    // 序号不连续
    if( quote.sequence_no != (symbol_meta.seq_no + 1) ) 
    {
        LOG_WARN(exchange + "." + symbol + " sequence is not continuous, symbol_meta.seq_no=" 
                + std::to_string(symbol_meta.seq_no) + ", quote.sequence_no: " + std::to_string(quote.sequence_no));
        
        redis_snap_requester_.async_request_symbol(exchange, symbol);
        symbol_meta.caches.clear();
        symbol_meta.caches.push_back(quote);
        symbol_meta.publishing = false;
        symbol_meta.seq_no = quote.sequence_no;

        meta.pkg_skip_count += 1;

        return SYNC_REQ_SNAP;
    }

    symbol_meta.seq_no = quote.sequence_no;

    if( symbol_meta.publishing )
    {
        return SYNC_OK; 
    }
    else
    {
        // 行情之前出现的seq 乱序，还在等待异步请求的 snap 到来
        // 这个比较是用来判断 当前的这个 quote 是否有存储的必要；
        if( quote.sequence_no > symbol_meta.snap.sequence_no )
        {
            symbol_meta.caches.push_back(quote);
        }
        return SYNC_SKIP;
    }
};

void RedisQuote::_looping() 
{
    last_statistic_time_ = get_miliseconds();
    last_nodata_time_ = get_miliseconds();
    

    while( true ) 
    {
        // last_statistic_time_str_ = utrade::pandora::SecTimeStr();

        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(_looping_check_secs));



        type_tick now = get_miliseconds();

        // 强制刷出没有后续更新的品种
        //_looping_force_to_update();
        
        // 检查心跳
        if( now > last_time_ && (now - last_time_) > 10 * 1000 ) {
            LOG_WARN("heartbeat expired. now="+ std::to_string(now) +", last=" + std::to_string(last_time_));
            _loopng_check_heartbeat();
        }

        // 检查异常没有数据的交易所
        if( (now - last_nodata_time_) > 30*1000 ) 
        {
            _looping_check_nodata();
            last_nodata_time_ = now;
        }

        // // 打印统计信息
        // if( (now - last_statistic_time_) > 10*1000 ) 
        // {
        //     _looping_print_statistics();
        //     last_statistic_time_ = now;
        // }

        _looping_print_statistics();
    }
}

void RedisQuote::_looping_force_to_update()
{
    type_tick now = get_miliseconds();
    SExchangeConfig config;
    vector<SDepthQuote> forced_to_update;

    {
        std::unique_lock<std::mutex> l{ mutex_clocks_ };
        for( auto& v : frequency_metas_ )
        {
            const TSymbol& symbol = v.first;
            _SFrequencyMeta& meta = v.second;
            for( auto& u : meta.updates )
            {                    
                const TExchange& exchange = u.first;
                SDepthQuote& cache = u.second;
                if( cache.asks.size() == 0 && cache.bids.size() == 0 )
                    continue;
                if( !_get_config(exchange, symbol, config) )
                    continue;
                    
                if( config.frequency != 0 && (now - meta.last_clocks[exchange]) < (1000/config.frequency) ) {
                    continue;
                }
                cout << now << " " << meta.last_clocks[exchange] << endl;
                meta.last_clocks[exchange] = now;
                
                SDepthQuote tmp;
                tmp.exchange = exchange;
                tmp.symbol = symbol;
                tmp.arrive_time = cache.arrive_time;
                tmp.asks.swap(cache.asks);
                tmp.bids.swap(cache.bids);
                forced_to_update.push_back(tmp);
            }
        }
    }

    for( const auto& v : forced_to_update ) 
    { 
        engine_interface_->on_update(v.exchange, v.symbol, v);
    }

}

void RedisQuote::_loopng_check_heartbeat()
{
    // 请求增量
    LOG_WARN("reconnect redis ...");
    
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);
    last_time_ = get_miliseconds();
    last_redis_time_ = get_miliseconds();

    LOG_WARN("reconnect redis ok.");
}

void RedisQuote::_looping_check_nodata()
{
    unordered_set<TExchange> nodata_exchanges;
    {
        std::unique_lock<std::mutex> l{ mutex_metas_ };
        for( const auto& v : metas_ ) {
            if( v.second.pkg_count == 0 ) {
                nodata_exchanges.insert(v.first);
                LOG_WARN(v.first + string(" no data"));
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

    // tfm::printfln("-------------");
    // ExchangeMeta total;
    // for( const auto& v : statistics ) {
    //     tfm::printfln("%s\t\t%s", v.first, v.second.get());
    //     total.accumlate(v.second);
    // }
    // tfm::printfln("total\t\t%s", total.get());
    // tfm::printfln("-------------");
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

bool RedisQuote::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    LOG_INFO("Set Config " + symbol + ":\n");
    for (auto iter:config)
    {
        LOG_INFO(iter.first + "\n" + iter.second.str());
    }

    // 取消订阅之前的数据（改为disable，不再取消）
    unordered_set<TExchange> subscribed_exchanges;
    {
        std::unique_lock<std::mutex> l{ mutex_symbol_ };
        const auto& iter = symbols_.find(symbol);
        if( iter != symbols_.end() )
        {
            for( const auto& v : iter->second ) 
            {
                const TExchange& exchange = v.first;
                if( v.second.enable )
                    subscribed_exchanges.insert(exchange);
                //unsubscribe(exchange, symbol);
            }
        }
        symbols_[symbol] = config;
    }

    // 清空metas_中的缓存
    {
        std::unique_lock<std::mutex> l{ mutex_metas_ };
        auto iter = metas_.find(symbol);
        if( iter != metas_.end() )
        {
            iter->second.exchanges.clear();
        }
    }

    // 新增订阅
    for( const auto& v : config )
    {
        const TExchange& exchange = v.first;
        if( subscribed_exchanges.find(exchange) == subscribed_exchanges.end() )
        {
            LOG_INFO("subscribe new: " + exchange + "." +symbol);
            subscribe(exchange, symbol);
        }
            
    }
    return true;
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

SDepthQuote RedisQuote::merge_update(list<SDepthQuote>& updates_queue, SDepthQuote& snap)
{
    try
    {
        SDepthQuote merged_update = snap;
        merged_update.asks.clear();
        merged_update.bids.clear();
        for( const auto& v: updates_queue ) 
        {
            merged_update.arrive_time = v.arrive_time;
            merged_update.server_time = v.server_time;
            merged_update.sequence_no = v.sequence_no;
            update_depth_diff(v.asks, merged_update.asks);
            update_depth_diff(v.bids, merged_update.bids);
        }

        return merged_update;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

SDepthQuote RedisQuote::merge_quote(list<SDepthQuote>& quote_list)
{
    try
    {
        SDepthQuote result;
        result.asks.clear();
        result.bids.clear();
        for( const auto& v: quote_list ) 
        {
            result.arrive_time = v.arrive_time;
            result.server_time = v.server_time;
            result.sequence_no = v.sequence_no;
            result.exchange = v.exchange;
            result.symbol = v.symbol;
            update_depth_diff(v.asks, result.asks);
            update_depth_diff(v.bids, result.bids);
        }

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}