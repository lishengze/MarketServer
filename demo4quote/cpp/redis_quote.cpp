#include "redis_quote.h"
#include "stream_engine_config.h"

void redisquote_to_quote_depth(const njson& data, const RedisQuote::ExchangeConfig& config, map<SDecimal, SDecimal>& depths)
{
    for (auto iter = data.begin() ; iter != data.end() ; ++iter )
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        SDecimal dPrice = SDecimal::parse(price, -1);
        SDecimal dVolume = SDecimal::parse(volume, -1);
        depths[dPrice] = dVolume;
        //cout << price << "\t" << dPrice.get_str_value() << "\t" << volume << "\t" << dVolume.get_str_value() << endl;
    }
}

bool redisquote_to_quote(const njson& snap_json, SDepthQuote& quote, const RedisQuote::ExchangeConfig& config, bool isSnap) {
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq_symbol"].get<long long>(); 
    
    quote.exchange = exchange;
    quote.symbol = symbol;
    vassign(quote.sequence_no, sequence_no);
    //vassign(quote.time_arrive, timeArrive); // 暂时不处理
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth], config, quote.asks);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth], config, quote.bids);
    return true;
}

bool redisquote_to_kline(const njson& data, KlineData& kline) 
{
    kline.index = int(data[0].get<double>());
    kline.px_open.from(data[1].get<double>());
    kline.px_high.from(data[2].get<double>());
    kline.px_low.from(data[3].get<double>());
    kline.px_close.from(data[4].get<double>());
    kline.volume.from(data[5].get<double>());
    return true;
}

bool redisquote_to_trade(const njson& data, Trade& trade) 
{
    //trade.time = int(data[0].get<double>());
    trade.price.from(data["LastPx"].get<double>());
    trade.volume.from(data["Qty"].get<double>());
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
    njson snap_json;
    try
    {
        snap_json = njson::parse(data);
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("parse json fail %s", e.what());
        return false;
    } 

    ExchangeConfig config;
    if( !_get_symbol_config(exchange, symbol, config) ) {            
        _log_and_print("symbol not exist. exchange=%s symbol=%s", exchange.c_str(), symbol.c_str());
        return false;
    }
    SDepthQuote quote;
    if( !redisquote_to_quote(snap_json, quote, config, true))
    {
        _log_and_print("%s-%s redisquote_to_quote failed. msg=%s", exchange.c_str(), symbol.c_str(), data.c_str());
        return false;
    }
    quote.raw_length = data.length();

    // 更新meta信息
    list<SDepthQuote> wait_to_send;
    if( _update_meta_by_snap(quote.exchange, quote.symbol, quote, wait_to_send) ){
        // 执行发送
        engine_interface_->on_snap(exchange, symbol, quote);
        for( const auto& v: wait_to_send ) {
            _ctrl_update(exchange, symbol, v);
        }
        return true;
    }

    return true;
};

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg)
{
    njson snap_json;

    // 设置最近数据时间
    last_time_ = get_miliseconds();

    if (channel.find(DEPTH_UPDATE_HEAD) != string::npos)
    {
        // 
        int pos = channel.find(DEPTH_UPDATE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(DEPTH_UPDATE_HEAD), pos2-pos-strlen(DEPTH_UPDATE_HEAD));
        string exchange = channel.substr(pos2+1);

        // 解析json
        try
        {
            snap_json = njson::parse(msg);
        }
        catch(nlohmann::detail::exception& e)
        {
            _log_and_print("parse json fail %s", e.what());
            return;
        } 

        // redis结构到内部结构的转换
        ExchangeConfig config;
        if( !_get_symbol_config(exchange, symbol, config) ) {            
            _log_and_print("channel=%s symbol not exist. exchange=%s symbol=%s", channel.c_str(), exchange.c_str(), symbol.c_str());
            return;
        }
        SDepthQuote quote;
        if( !redisquote_to_quote(snap_json, quote, config, false)) 
        {
            _log_and_print("channel=%s redisquote_to_quote failed. msg=%s", channel.c_str(), msg.c_str());
            return;
        }
        quote.raw_length = msg.length();

        // 更新meta信息
        if( !_update_meta_by_update(quote.exchange, quote.symbol, quote) ) {
            return;
        }

        // 数据更新需要通过频率控制，所以调用ctrl_update
        _ctrl_update(quote.exchange, quote.symbol, quote) ;
    }
    else if(channel.find(TRADE_HEAD) != string::npos )
    {
        //cout << msg << endl;
        // 
        int pos = channel.find(TRADE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(TRADE_HEAD), pos2-pos-strlen(TRADE_HEAD));
        string exchange = channel.substr(pos2+1);

        // 解析json
        try
        {
            snap_json = njson::parse(msg);
        }
        catch(nlohmann::detail::exception& e)
        {
            _log_and_print("parse json fail %s", e.what());
            return;
        } 

        Trade trade;
        redisquote_to_trade(snap_json, trade);
        engine_interface_->on_trade(exchange, symbol, trade);
    }
    else if(channel.find(KLINE_1MIN_HEAD) != string::npos)
    {
        // 
        int pos = channel.find(KLINE_1MIN_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(KLINE_1MIN_HEAD), pos2-pos-strlen(KLINE_1MIN_HEAD));
        string exchange = channel.substr(pos2+1);
        
        // 解析json
        try
        {
            snap_json = njson::parse(msg);
        }
        catch(nlohmann::detail::exception& e)
        {
            _log_and_print("parse json fail %s", e.what());
            return;
        } 

        // 后续更新只取最近一根
        if( kline1min_firsttime_[symbol][exchange] == true )
        {
            vector<KlineData> datas;
            KlineData kline;
            for (auto iter = snap_json.rbegin(); iter != snap_json.rend(); ++iter) {
                redisquote_to_kline(*iter, kline);
                _log_and_print("get kline %s index=%lu open=%s high=%s low=%s close=%s volume=%s", channel.c_str(), 
                    kline.index,
                    kline.px_open.get_str_value().c_str(),
                    kline.px_high.get_str_value().c_str(),
                    kline.px_low.get_str_value().c_str(),
                    kline.px_close.get_str_value().c_str(),
                    kline.volume.get_str_value().c_str()
                    );
                datas.push_back(kline);
                break;
            }
            engine_interface_->on_kline(exchange, symbol, 60, datas, false);
        }
        // 首次更新取所有数据
        else 
        {
            vector<KlineData> datas;
            KlineData kline;
            for (auto iter = snap_json.begin(); iter != snap_json.end(); ++iter) {
                redisquote_to_kline(*iter, kline);
                datas.push_back(kline);
            }
            _log_and_print("get kline %s firsttime, update %lu records.", channel.c_str(), datas.size());
            engine_interface_->on_kline(exchange, symbol, 60, datas, true);
            kline1min_firsttime_[symbol][exchange] = true;
        }
    }
    else if( channel.find(KLINE_60MIN_HEAD) != string::npos )
    {
        // 
        int pos = channel.find(KLINE_60MIN_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(KLINE_60MIN_HEAD), pos2-pos-strlen(KLINE_60MIN_HEAD));
        string exchange = channel.substr(pos2+1);
        
        // 解析json
        try
        {
            snap_json = njson::parse(msg);
        }
        catch(nlohmann::detail::exception& e)
        {
            _log_and_print("parse json fail %s", e.what());
            return;
        } 

        // 后续更新只取最近一根
        if( kline60min_firsttime_[symbol][exchange] == true )
        {
            vector<KlineData> datas;
            KlineData kline;
            for (auto iter = snap_json.rbegin(); iter != snap_json.rend(); ++iter) {
                redisquote_to_kline(*iter, kline);
                datas.push_back(kline);
                break;
            }
            engine_interface_->on_kline(exchange, symbol, 3600, datas, false);
        }
        // 首次更新取所有数据
        else 
        {
            vector<KlineData> datas;
            KlineData kline;
            for (auto iter = snap_json.begin(); iter != snap_json.end(); ++iter) {
                redisquote_to_kline(*iter, kline);
                datas.push_back(kline);
            }
            _log_and_print("get kline60 %s firsttime, update %lu records.", channel.c_str(), datas.size());
            engine_interface_->on_kline(exchange, symbol, 3600, datas, true);
            kline60min_firsttime_[symbol][exchange] = true;
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
                redis_snap_requester_.request_symbol(exchange, symbol);
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
            _log_and_print("%s-%s sequence skip from %lu to %lu. stop and request snap again ...", 
                            exchange.c_str(), symbol.c_str(), symbol_meta.seq_no, quote.sequence_no);
            // 序号不连续
            redis_snap_requester_.request_symbol(exchange, symbol);
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
                redis_snap_requester_.request_symbol(exchange, symbol);
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
                redis_snap_requester_.request_symbol(exchange, symbol);
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

void RedisQuote::set_config(const TSymbol& symbol, float frequency, const ExchangeConfigs& exchanges)
{
    // 打印入参
    string desc = "";
    for( const auto& v : exchanges ) {
        desc += v.first + ",";
    }
    _log_and_print("%s frequency=%.05f, exchanges=%s", symbol.c_str(), frequency, desc.c_str());

    // 设置交易所配置
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_symbol_ };
        // 删除老的
        const auto& iter = symbols_.find(symbol);
        if( iter != symbols_.end() )
        {
            for( const auto& v : iter->second ) {
                unsubscribe(v.first, symbol);
            }
        }

        // 添加新的    
        for( const auto& v : exchanges )
        {
            subscribe(v.first, symbol);
        }

        // 赋值
        symbols_[symbol] = exchanges;
    }
    
    // 设置原始行情频率配置
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
        frequecy_[symbol] = frequency;
    }
}

bool RedisQuote::_get_symbol_config(const TExchange& exchange, const TSymbol& symbol, ExchangeConfig& config) const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_symbol_ };
    auto v = symbols_.find(symbol);
    if( v == symbols_.end() )
        return false;
    const ExchangeConfigs& configs = v->second;        
    auto v2 = configs.find(exchange);
    if( v2 == configs.end() )
        return false;
    config = v2->second;
    return true;
}