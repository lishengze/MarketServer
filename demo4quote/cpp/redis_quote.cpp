#include "redis_quote.h"
#include "stream_engine_config.h"

void redisquote_to_quote_depth(const njson& data, SDepthPrice* depth, unsigned int& depth_length, bool is_ask)
{
    map<SDecimal, double> depths;
    for (auto iter = data.begin() ; iter != data.end() ; ++iter )
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        SDecimal dPrice;
        dPrice.from(price, -1);
        depths[dPrice] = volume;
    }

    int count = 0;
    if( is_ask ) {
        for( auto iter = depths.begin() ; count < MAX_DEPTH && iter != depths.end() ; ++iter, ++count )
        {
            depth[count].price = iter->first;
            depth[count].volume = iter->second;
        }
    } else {
        for( auto iter = depths.rbegin() ; count < MAX_DEPTH && iter != depths.rend() ; ++iter, ++count )
        {
            depth[count].price = iter->first;
            depth[count].volume = iter->second;
        }
    }
    depth_length = count;
}

bool redisquote_to_quote(const string& data, SDepthQuote& quote, bool isSnap) {
    njson snap_json = njson::parse(data); 
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq_symbol"].get<long long>(); 
    
    vassign(quote.exchange, MAX_EXCHANGE_NAME_LENGTH, exchange);
    vassign(quote.symbol, MAX_SYMBOL_NAME_LENGTH, symbol);
    vassign(quote.sequence_no, sequence_no);
    //vassign(quote.time_arrive, timeArrive); // 暂时不处理
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    redisquote_to_quote_depth(snap_json[askDepth], quote.asks, quote.ask_length, true);
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    redisquote_to_quote_depth(snap_json[bidDepth], quote.bids, quote.bid_length, false);

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
        _log_and_print("RedisQuote::_on_snap: %s-%s size=0.", exchange.c_str(), symbol.c_str());
        return false;
    }

    SDepthQuote quote;
    if( !redisquote_to_quote(data, quote, true))
    {
        _log_and_print("RedisQuote::_on_snap: %s-%s redisquote_to_quote failed. msg=%s", exchange.c_str(), symbol.c_str(), data.c_str());
        return false;
    }

    // 更新meta信息
    list<SDepthQuote> wait_to_send;
    if( _snap_meta(quote.exchange, quote.symbol, quote, wait_to_send) ){
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
    
    //std::cout << "update json size:" << msg.length() << std::endl;
    if (channel.find(DEPTH_UPDATE_HEAD) != string::npos)
    {
        // DEPTH_UPDATE_HEAD频道不存在空包
        if( msg.length() == 0 ) 
        {
            _log_and_print("RedisQuote::OnMessage: channel=%s msg length = 0.", channel.c_str());
            return;
        }

        // redis结构到内部结构的转换
        SDepthQuote quote;
        if( !redisquote_to_quote(msg, quote, false)) 
        {
            _log_and_print("RedisQuote::OnMessage: channel=%s redisquote_to_quote failed. msg=%s", channel.c_str(), msg.c_str());
            return;
        }

        // 更新meta信息
        if( !_update_meta(quote.exchange, quote.symbol, quote) ) {
            return;
        }

        // 回调
        //engine_interface_->on_update(quote.exchange, quote.symbol, quote);
        if( !_ctrl_update(quote.exchange, quote.symbol, quote) ) {         
            _log_and_print("RedisQuote::OnMessage: channel=%s update too many depth.", channel.c_str());   
            redis_snap_requester_.add_symbol(quote.exchange, quote.symbol);
        }
    }
    else if(channel.find(TICK_HEAD) != string::npos)
    {
    }
    else
    {
        _log_and_print("RedisQuote::OnMessage: channel=%s Unknown Message Type", channel.c_str());
    }
};

void RedisQuote::OnConnected() {
    _log_and_print("Redis RedisQuote::OnConnected");
    engine_interface_->on_connected();
};

void RedisQuote::OnDisconnected(int status) {
    _log_and_print("Redis RedisQuote::OnDisconnected");
    engine_interface_->on_disconnected();
};

bool RedisQuote::_snap_meta(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, list<SDepthQuote>& wait_to_send)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };

    ExchangeMeta& meta = metas_[exchange];
    SymbolMeta& symbol_meta = meta.symbols[symbol];

    if( symbol_meta.seq_no > 0 ) {
        // 已经开始推送
        _log_and_print("RedisQuote::_snap_meta: %s-%s recv snap again.", exchange.c_str(), symbol.c_str());
        symbol_meta.seq_no = 0;
        symbol_meta.snap = quote;
        return false;
    } else {
        // 未开始推送
        if( symbol_meta.caches.size() == 0 ) {
            // 没有缓存
            _log_and_print("RedisQuote::_snap_meta: %s-%s updates is empty.", exchange.c_str(), symbol.c_str());
            symbol_meta.snap = quote;
            return false;
        } else {
            _log_and_print("RedisQuote::_snap_meta: %s-%s updates=%lu-%lu snap=%lu", exchange.c_str(), symbol.c_str(), 
                symbol_meta.caches.front().sequence_no,
                symbol_meta.caches.back().sequence_no, 
                quote.sequence_no);

            if( quote.sequence_no < (symbol_meta.caches.front().sequence_no-1) ) {
                // snap < head 
                _log_and_print("RedisQuote::_snap_meta: %s-%s snap too late.", exchange.c_str(), symbol.c_str());
                redis_snap_requester_.add_symbol(exchange, symbol);
                return false;
            } else if( quote.sequence_no <= symbol_meta.caches.back().sequence_no ) {
                // snap < tail
                list<SDepthQuote> wait_to_send; // 第一个为全量包，后续为增量包
                for( const auto& v : symbol_meta.caches ) 
                {
                    if( v.sequence_no >= (quote.sequence_no + 1 ) ) {
                        wait_to_send.push_back(v);
                    }
                }
                _log_and_print("RedisQuote::_snap_meta: %s-%s updates %lu records.", exchange.c_str(), symbol.c_str(), wait_to_send.size());
                symbol_meta.caches.clear();
                symbol_meta.seq_no = wait_to_send.size() == 0 ? quote.sequence_no : wait_to_send.back().sequence_no;
                symbol_meta.snap = quote;
                return true;
            } else {
                // snap > tail
                _log_and_print("RedisQuote::_snap_meta: %s-%s updates too late.", exchange.c_str(), symbol.c_str());
                symbol_meta.snap = quote;
                symbol_meta.caches.clear();
                return false;
            }
        } 
    }

    // never reach here
    return true;
}

bool RedisQuote::_update_meta(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };

    ExchangeMeta& meta = metas_[exchange];
    SymbolMeta& symbol_meta = meta.symbols[symbol];

    // 更新统计信息
    meta.pkg_count += 1;
    meta.pkg_size += quote.raw_length;

    if( symbol_meta.seq_no > 0 )
    {   // 已经开始下发
        if( quote.sequence_no == (symbol_meta.seq_no + 1) ) {
            // 序号连续
            symbol_meta.seq_no = quote.sequence_no;
            return true;
        } else {
            // 序号不连续
            redis_snap_requester_.add_symbol(exchange, symbol);
            symbol_meta.caches.clear();
            symbol_meta.caches.push_back(quote);
            symbol_meta.seq_no = 0;
            symbol_meta.snap.sequence_no = 0;

            meta.pkg_skip_count += 1;
            _log_and_print("RedisQuote::_update_meta: %s-%s sequence skip from %lu to %lu", exchange.c_str(), symbol.c_str(), symbol_meta.seq_no, quote.sequence_no);
        
            return false;
        }
    }
    else
    {   // 未开始下发
        auto last_snap_seqno = symbol_meta.snap.sequence_no;
        if( last_snap_seqno == 0 ) {
            // snap还没有到
            auto last_seqno = symbol_meta.caches.size() == 0 ? 0 : symbol_meta.caches.back().sequence_no;
            if( last_seqno == 0 || (last_seqno+1) == quote.sequence_no ) {
                // 连续
                redis_snap_requester_.add_symbol(exchange, symbol);
                symbol_meta.caches.push_back(quote);
                return false;
            } else {
                // 不连续
                _log_and_print("RedisQuote::_update_meta: %s-%s sequence skip from %lu to %lu", exchange.c_str(), symbol.c_str(), last_seqno, quote.sequence_no);
                symbol_meta.caches.clear();
                symbol_meta.caches.push_back(quote);

                meta.pkg_skip_count += 1;
        
                return false;
            }
        } else {
            _log_and_print("RedisQuote::_update_meta: %s-%s snap=%lu update=%lu", exchange.c_str(), symbol.c_str(), last_snap_seqno, quote.sequence_no);
            if( (last_snap_seqno+1) == quote.sequence_no ) {
                _log_and_print("RedisQuote::_update_meta: %s-%s match.", exchange.c_str(), symbol.c_str());
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

void RedisQuote::subscribe(const string& channel) {
    redis_api_->SubscribeTopic(channel);
};

void RedisQuote::psubscribe(const string& pchannel) {
    redis_api_->PSubscribeTopic(pchannel);
};

void RedisQuote::_check() 
{
    last_statistic_time_ = get_miliseconds();
    last_nodata_time_ = get_miliseconds();

    while( true ) 
    {
        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(1));

        type_tick now = get_miliseconds();
        // 检查心跳
        if( (now - last_time_) > 10 * 1000 ) {
            _log_and_print("RedisQuote::_check: heartbeat expired. %ld:%ld", now, last_time_);

            // 请求增量
            std::cout << "reconnect redis ..." << std::endl;
            redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
            redis_api_->RegisterSpi(this);
            redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);

            last_time_ = now;
            last_redis_time_ = now;
            std::cout << "reconnect redis ok." << std::endl;
        }

        // 检查异常没有数据的交易所
        if( (now - last_nodata_time_) > 30*1000 ) 
        {
            unordered_set<TExchange> nodata_exchanges;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_metas_ };
                for( const auto& v : metas_ ) {
                    if( v.second.pkg_count == 0 ) {
                        nodata_exchanges.insert(v.first);
                        _log_and_print("RedisQuote::_check: clear exchange %s", v.first.c_str());
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
    _UpdateDepth& update = updates_[exchange][symbol];
    for( unsigned int i = 0 ; i < quote.ask_length ; ++i ) {
        const SDepthPrice& depth = quote.asks[i];
        update.asks[depth.price] = depth.volume;
    }
    for( unsigned int i = 0 ; i < quote.bid_length ; ++i ) {
        const SDepthPrice& depth = quote.bids[i];
        update.bids[depth.price] = depth.volume;
    }
    if( !_check_update_clocks(exchange, symbol) ) {
        return true;
    }

    // 数据太多
    if( update.asks.size() >= MAX_DEPTH || update.bids.size() >= MAX_DEPTH ) {
        update.asks.clear();
        update.bids.clear();
        return false;
    }

    SDepthQuote tmp;
    vassign(tmp.exchange, MAX_EXCHANGE_NAME_LENGTH, exchange);
    vassign(tmp.symbol, MAX_SYMBOL_NAME_LENGTH, symbol);
    int count = 0;
    for( auto iter = update.asks.begin() ; iter != update.asks.end() ; ++iter, ++count )
    {
        tmp.asks[count].price = iter->first;
        tmp.asks[count].volume = iter->second;
    }
    tmp.ask_length = count;
    count = 0;
    for( auto iter = update.bids.rbegin() ; iter != update.bids.rend() ; ++iter, ++count )
    {
        tmp.bids[count].price = iter->first;
        tmp.bids[count].volume = iter->second;
    }
    tmp.bid_length = count;
    engine_interface_->on_update(exchange, symbol, tmp);

    update.asks.clear();
    update.bids.clear();
    return true;
}

bool RedisQuote::_check_update_clocks(const TExchange& exchange, const TSymbol& symbol) {
    if( CONFIG->grpc_publish_raw_frequency_ == 0 )
        return true;

    // 每秒更新频率控制
    auto last = last_clocks_[exchange][symbol];
    auto now = get_miliseconds();
    if( (now - last) < (1000/CONFIG->grpc_publish_raw_frequency_) ) {
        return false;
    }
    last_clocks_[exchange][symbol] = now;
    return true;
}