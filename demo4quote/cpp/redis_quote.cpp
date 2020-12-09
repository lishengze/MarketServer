#include "redis_quote.h"
#include "stream_engine_config.h"

void redisquote_to_quote_depth(const njson& data, map<SDecimal, SDecimal>& depths)
{
    for (auto iter = data.begin() ; iter != data.end() ; ++iter )
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        SDecimal dPrice = SDecimal::parse(price);
        depths[dPrice] = SDecimal::parse(volume);
    }
}

bool redisquote_to_quote(const string& data, SDepthQuote& quote, bool isSnap) {
    njson snap_json = njson::parse(data); 
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq_symbol"].get<long long>(); 
    
    quote.exchange = exchange;
    quote.symbol = symbol;
    vassign(quote.sequence_no, sequence_no);
    //vassign(quote.time_arrive, timeArrive); // 暂时不处理
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth], quote.asks);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth], quote.bids);

    quote.raw_length = data.length();
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

void RedisQuote::start(const RedisParams& params, UTLogPtr logger) {
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
    long long now = get_miliseconds();
    last_time_ = now;
    checker_loop_ = new std::thread(&RedisQuote::_check, this);
};

bool RedisQuote::_on_snap(const TExchange& exchange, const TSymbol& symbol, const string& data) 
{    
    if( data.length() == 0 )
    {
        _log_and_print("%s-%s size=0.", exchange.c_str(), symbol.c_str());
        return true;
    }

    SDepthQuote quote;
    if( !redisquote_to_quote(data, quote, true))
    {
        _log_and_print("%s-%s redisquote_to_quote failed. msg=%s", exchange.c_str(), symbol.c_str(), data.c_str());
        return false;
    }

    // 更新meta信息
    list<SDepthQuote> wait_to_send;
    if( _update_meta_by_snap(quote.exchange, quote.symbol, quote, wait_to_send) ){
        // 执行发送
        engine_interface_->on_snap(exchange, symbol, quote);
        for( const auto& v: wait_to_send ) {
            //engine_interface_->on_update(exchange, symbol, v);
            _ctrl_update(exchange, symbol, v);
        }
        return true;
    }

    return true;
};

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg)
{
    // 设置最近数据时间
    last_time_ = get_miliseconds();
    //cout << channel << endl;
    
    //_log_and_print("%s update json size %lu", channel.c_str(), msg.length());
    //std::cout << "update json size:" << msg.length() << std::endl;
    if (channel.find(DEPTH_UPDATE_HEAD) != string::npos)
    {
        // DEPTH_UPDATE_HEAD频道不存在空包
        if( msg.length() == 0 ) 
        {
            _log_and_print("channel=%s msg length = 0.", channel.c_str());
            return;
        }

        // redis结构到内部结构的转换
        SDepthQuote quote;
        if( !redisquote_to_quote(msg, quote, false)) 
        {
            _log_and_print("channel=%s redisquote_to_quote failed. msg=%s", channel.c_str(), msg.c_str());
            return;
        }

        // 更新meta信息
        if( !_update_meta_by_update(quote.exchange, quote.symbol, quote) ) {
            return;
        }

        // 回调
        //engine_interface_->on_update(quote.exchange, quote.symbol, quote);
        _ctrl_update(quote.exchange, quote.symbol, quote) ;
    }
    else if(channel.find(KLINE_1MIN_HEAD) != string::npos)
    {
        // decode exchange and symbol
        int pos = channel.find(KLINE_1MIN_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(KLINE_1MIN_HEAD), pos2-pos-strlen(KLINE_1MIN_HEAD));
        string exchange = channel.substr(pos2+1);

        njson snap_json = njson::parse(msg); 

        for (auto iter = snap_json.rbegin(); iter != snap_json.rend(); ++iter) {
            // 取最后一根
            const njson& data = *iter;
            KlineData kline;
            kline.index = int(data[0].get<float>());
            kline.px_open.from(data[1].get<double>());
            kline.px_high.from(data[2].get<double>());
            kline.px_low.from(data[3].get<double>());
            kline.px_close.from(data[4].get<double>());
            kline.volume.from(data[5].get<double>());
            _log_and_print("get kline %s index=%lu open=%s high=%s low=%s close=%s volume=%s", channel.c_str(), 
                kline.index,
                kline.px_open.get_str_value().c_str(),
                kline.px_high.get_str_value().c_str(),
                kline.px_low.get_str_value().c_str(),
                kline.px_close.get_str_value().c_str(),
                kline.volume.get_str_value().c_str()
                );
            vector<KlineData> datas;
            datas.push_back(kline);
            engine_interface_->on_kline(exchange, symbol, 60, datas);
            break;
        }
    }
    else
    {
        _log_and_print("channel=%s Unknown Message Type", channel.c_str());
    }
};

void RedisQuote::OnConnected() {
    _log_and_print("Redis RedisQuote::OnConnected");
};

void RedisQuote::OnDisconnected(int status) {
    _log_and_print("Redis RedisQuote::OnDisconnected");
};

bool RedisQuote::_update_meta_by_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, list<SDepthQuote>& wait_to_send)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.symbols[exchange];

    if( symbol_meta.publish_started() ) 
    {
        // 已经开始推送
        _log_and_print("%s-%s recv snap again. stop and wait for updates ...", exchange.c_str(), symbol.c_str());
        symbol_meta.seq_no = 0;
        symbol_meta.snap = quote;
        return false;
    } 
    else 
    {
        // 未开始推送
        if( symbol_meta.caches.size() == 0 ) 
        {
            // 没有缓存
            //_log_and_print("%s-%s updates is empty. wait for updates ...", exchange.c_str(), symbol.c_str());
            _log_and_print("%s-%s start to publish.", exchange.c_str(), symbol.c_str());
            symbol_meta.snap = quote;
            symbol_meta.seq_no = quote.sequence_no;
            return true;
        } 
        else 
        {
            _log_and_print("%s-%s updates from %lu to %lu snap is %lu ...", exchange.c_str(), symbol.c_str(), 
                symbol_meta.caches.front().sequence_no,
                symbol_meta.caches.back().sequence_no, 
                quote.sequence_no);

            if( quote.sequence_no < (symbol_meta.caches.front().sequence_no-1) ) 
            {
                // snap < head 
                _log_and_print("%s-%s snap too late. request snap again.", exchange.c_str(), symbol.c_str());
                redis_snap_requester_.add_symbol(exchange, symbol);
                return false;
            } 
            else if( quote.sequence_no <= symbol_meta.caches.back().sequence_no ) 
            {
                // snap < tail
                list<SDepthQuote> wait_to_send; // 第一个为全量包，后续为增量包
                for( const auto& v : symbol_meta.caches ) 
                {
                    if( v.sequence_no >= (quote.sequence_no + 1 ) ) {
                        wait_to_send.push_back(v);
                    }
                }
                _log_and_print("%s-%s start to publish.", exchange.c_str(), symbol.c_str());
                symbol_meta.caches.clear();
                symbol_meta.seq_no = wait_to_send.size() == 0 ? quote.sequence_no : wait_to_send.back().sequence_no;
                symbol_meta.snap = quote;
                return true;
            } 
            else 
            {
                // snap > tail
                _log_and_print("%s-%s updates too late. wait for updates.", exchange.c_str(), symbol.c_str());
                symbol_meta.snap = quote;
                symbol_meta.caches.clear();
                return false;
            }
        } 
    }

    // never reach here
    return true;
}

bool RedisQuote::_update_meta_by_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };

    ExchangeMeta& meta = metas_[symbol];
    SymbolMeta& symbol_meta = meta.symbols[exchange];

    // 更新统计信息
    meta.pkg_count += 1;
    meta.pkg_size += quote.raw_length;

    if( symbol_meta.publish_started() )
    {   // 已经开始下发
        if( quote.sequence_no == (symbol_meta.seq_no + 1) ) {
            // 序号连续
            symbol_meta.seq_no = quote.sequence_no;
            return true;
        } else {
            _log_and_print("%s-%s sequence skip from %lu to %lu. stop and request snap again ...", exchange.c_str(), symbol.c_str(), symbol_meta.seq_no, quote.sequence_no);
            // 序号不连续
            redis_snap_requester_.add_symbol(exchange, symbol);
            symbol_meta.caches.clear();
            symbol_meta.caches.push_back(quote);
            symbol_meta.seq_no = 0;
            symbol_meta.snap.sequence_no = 0;

            meta.pkg_skip_count += 1;
        
            return false;
        }
    }
    else
    {   // 未开始下发
        auto last_snap_seqno = symbol_meta.snap.sequence_no;
        if( last_snap_seqno == 0 ) 
        {
            // snap还没有到
            auto last_seqno = symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.back().sequence_no;
            if( last_seqno == 0 || (last_seqno+1) == quote.sequence_no ) {
                // 连续
                redis_snap_requester_.add_symbol(exchange, symbol);
                symbol_meta.caches.push_back(quote);
                return false;
            } else {
                // 不连续
                _log_and_print("%s-%s sequence skip from %lu to %lu.", exchange.c_str(), symbol.c_str(), last_seqno, quote.sequence_no);
                symbol_meta.caches.clear();
                symbol_meta.caches.push_back(quote);

                meta.pkg_skip_count += 1;
        
                return false;
            }
        } else {
            _log_and_print("%s-%s snap is %lu update is %lu ...", exchange.c_str(), symbol.c_str(), last_snap_seqno, quote.sequence_no);
            if( (last_snap_seqno+1) == quote.sequence_no ) {
                _log_and_print("%s-%s start to publish.", exchange.c_str(), symbol.c_str());
                // 等于snap+1
                symbol_meta.seq_no = quote.sequence_no;
                symbol_meta.caches.clear();
                // 释放锁，发送snap
                inner_lock.unlock();
                engine_interface_->on_snap(exchange, symbol, symbol_meta.snap);
                return true;
            } else if( (last_snap_seqno+1) > quote.sequence_no ) {
                // 小于snap+1
                return false;
            } else {
                _log_and_print("%s-%s snap too late. request snap again.", exchange.c_str(), symbol.c_str());
                // 大于snap+1
                redis_snap_requester_.add_symbol(exchange, symbol);
                symbol_meta.caches.clear();
                symbol_meta.caches.push_back(quote);
                return false;
            }
        }
    }
    
    // never reach here
    return true;
};

void RedisQuote::_check() 
{
    last_statistic_time_ = get_miliseconds();
    last_nodata_time_ = get_miliseconds();

    while( true ) 
    {
        // 休眠
        std::this_thread::sleep_for(std::chrono::microseconds(10));

        type_tick now = get_miliseconds();

        // 强制刷出没有后续更新的品种
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

        // 检查心跳
        if( now > last_time_ && (now - last_time_) > 10 * 1000 ) {
            _log_and_print("heartbeat expired. now is %ld, last is %ld", now, last_time_);

            // 请求增量
            _log_and_print("reconnect redis ...");
            for( const auto& v : subscribed_topics_ ) {
                redis_api_->UnSubscribeTopic(v);
                redis_api_->SubscribeTopic(v);
            }
            //redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
            //redis_api_->RegisterSpi(this);
            //redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);

            last_time_ = now;
            last_redis_time_ = now;
            _log_and_print("reconnect redis ok.");
        }

        // 检查异常没有数据的交易所
        if( false && (now - last_nodata_time_) > 30*1000 ) 
        {
            unordered_set<TExchange> nodata_exchanges;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };
                for( const auto& v : metas_ ) {
                    if( v.second.pkg_count == 0 ) {
                        nodata_exchanges.insert(v.first);
                        _log_and_print("clear exchange %s", v.first.c_str());
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
            last_nodata_time_ = now;
        }

        // 打印统计信息
        if( (now - last_statistic_time_) > 10*1000 ) 
        {
            unordered_map<TExchange, ExchangeMeta> statistics;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };
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
            last_statistic_time_ = now;
        }
    }
}

bool RedisQuote::_ctrl_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };

    // 保留更新
    SDepthQuote& update = updates_[symbol][exchange];
    update.arrive_time = quote.arrive_time;
    for( const auto& v : quote.asks )
    {
        update.asks[v.first] = v.second;
    }
    for( const auto& v : quote.bids )
    {
        update.bids[v.first] = v.second;
    }

    // 检查频率
    float frequency = frequecy_[symbol];
    if( !_check_update_clocks(exchange, symbol, frequency) ) {
        //_log_and_print("skip %s-%s update.", exchange.c_str(), symbol.c_str());
        return true;
    }

    // 生成新的增量
    SDepthQuote tmp;
    tmp.exchange = exchange;
    tmp.symbol = symbol;
    tmp.arrive_time = update.arrive_time;
    tmp.asks.swap(update.asks);
    tmp.bids.swap(update.bids);
    engine_interface_->on_update(exchange, symbol, tmp);
    return true;
}

bool RedisQuote::_check_update_clocks(const TExchange& exchange, const TSymbol& symbol, float frequency) {
    if( frequency == 0 )
        return true;

    // 每秒更新频率控制
    auto last = last_clocks_[symbol][exchange];
    auto now = get_miliseconds();
    if( (now - last) < (1000/frequency) ) {
        return false;
    }
    last_clocks_[symbol][exchange] = now;
    return true;
}

void RedisQuote::set_frequency(const TSymbol& symbol, float frequency)
{
    _log_and_print("%s frequency=%.03f", symbol.c_str(), frequency);

    std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
    frequecy_[symbol] = frequency;
}

void RedisQuote::set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
{
    string desc = "";
    for( const auto& v : exchanges ) {
        desc += v + ",";
    }
    _log_and_print("%s exchanges=%s", symbol.c_str(), desc.c_str());

    // 删除老的
    const auto& iter = symbols_.find(symbol);
    if( iter != symbols_.end() )
    {
        for( const auto& v : iter->second ) {
            unsubscribe(v, symbol);
        }
    }

    // 添加新的    
    for( const auto& v : exchanges )
    {
        subscribe(v, symbol);
    }

    // 赋值
    symbols_[symbol] = exchanges;
}