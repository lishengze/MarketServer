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
    redis_snap_requester_.reset_symbol();
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
    last_statistic_time_ = now;
    checker_loop_ = new std::thread(&RedisQuote::_check_heartbeat, this);
};

void RedisQuote::_on_snap(const TExchange& exchange, const TSymbol& symbol, const string& data) 
{
     _println_("on_snap %s:%s size = %ld", exchange.c_str(), symbol.c_str(), data.length());
    if( data.length() == 0 )
        return;

    UT_LOG_INFO_FMT(CONFIG->logger_, "%s:%s %s", exchange.c_str(), symbol.c_str(), data.c_str());
    SDepthQuote quote;
    if( !redisquote_to_quote(data, quote, true))
        return;

    // 保存快照
    _set(quote.exchange, quote.symbol, quote);

    // 回调
    engine_interface_->on_snap(quote.exchange, quote.symbol, quote);
};

bool filter_by_config(const TExchange& exchange, const TSymbol& symbol)
{
    return CONFIG->include_symbols_.find(symbol) == CONFIG->include_symbols_.end() || CONFIG->include_exchanges_.find(exchange) == CONFIG->include_exchanges_.end();
}

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg){
    if( msg.length() == 0 )
        return;
    //std::cout << "update json size:" << msg.length() << std::endl;
    if (channel.find(DEPTH_UPDATE_HEAD) != string::npos)
    {
        // 设置最近数据时间
        last_time_ = get_miliseconds();

        // redis结构到内部结构的转换
        SDepthQuote quote;
        if( !redisquote_to_quote(msg, quote, false)) {
            UT_LOG_ERROR(CONFIG->logger_, "redis OnMessage: redisquote_to_quote failed");
            return;
        }

        // 更新统计信息
        _update_statistics(quote.exchange, msg, quote);

        // 更新检查包连续性
        if( !_update_seqno(quote.exchange, quote.symbol, quote.sequence_no) ) {
            return;
        }

        // 根据配置过滤不需要的行情品种
        if( filter_by_config(quote.exchange, quote.symbol) )
            return;

        // 添加到全量请求任务中
        if( !_check_snap_received(quote.exchange, quote.symbol) ) {
            redis_snap_requester_.add_symbol(quote.exchange, quote.symbol);
            return;
        }

        // 回调
        engine_interface_->on_update(quote.exchange, quote.symbol, quote);
    }
    else if(channel.find(TICK_HEAD) != string::npos)
    {
    }
    else
    {
        UT_LOG_ERROR(CONFIG->logger_, "Unknown Message Type");
    }
};

void RedisQuote::OnConnected() {
    cout << "\n##### Redis RedisQuote::OnConnected ####\n" << endl;
    UT_LOG_ERROR(CONFIG->logger_, "Redis RedisQuote::OnConnected");
    engine_interface_->on_connected();
};

void RedisQuote::OnDisconnected(int status) {
    cout << "\n##### Redis RedisQuote::OnDisconnected ####\n" << endl;
    UT_LOG_ERROR(CONFIG->logger_, "Redis RedisQuote::OnDisconnected");
    engine_interface_->on_disconnected();
};

bool RedisQuote::_update_seqno(const TExchange& exchange, const TSymbol& symbol, type_seqno sequence_no) {
    string symbolkey = make_symbolkey(exchange, symbol);
    auto iter = symbol_seqs_.find(symbolkey);
    if( iter == symbol_seqs_.end() )
        return true;
    if( sequence_no != (iter->second + 1) ) {
        char content[1024];
        sprintf(content,  "%s sequence skip %llu - %llu", symbolkey.c_str(), iter->second, sequence_no);
        std:: cout << content << std::endl;
        return false;
    }    
    symbol_seqs_[symbolkey] = sequence_no;
    return true;
};

bool RedisQuote::_check_snap_received(const TExchange& exchange, const TSymbol& symbol) const
{
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    auto iter = markets_.find(exchange);
    if( iter == markets_.end() )
        return false;
    const TMarketQuote& quotes = iter->second;
    auto iter2 = quotes.find(symbol);
    if( iter2 == quotes.end() )
        return false;
    return true;
}

void RedisQuote::_set(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    markets_[exchange][symbol] = quote;
}

void RedisQuote::subscribe(const string& channel) {
    redis_api_->SubscribeTopic(channel);
};

void RedisQuote::psubscribe(const string& pchannel) {
    redis_api_->PSubscribeTopic(pchannel);
};

void RedisQuote::_check_heartbeat() 
{
    while( true ) 
    {
        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(1));

        type_tick now = get_miliseconds();
        // 检查心跳
        {
            if( (now - last_time_) > 10 * 1000 ) {
                char content[1024];
                sprintf(content, "heartbeat expired. %ld:%ld", now, last_time_);
                std::cout << content << std::endl;

                // 重置
                redis_snap_requester_.reset_symbol();

                // 请求增量
                std::cout << "reconnect redis ..." << std::endl;
                redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
                redis_api_->RegisterSpi(this);
                redis_api_->RegisterRedis(params_.host, params_.port, params_.password, utrade::pandora::RM_Subscribe);

                last_time_ = now;
                last_statistic_time_ = now;
                std::cout << "reconnect redis ok." << std::endl;
            } else {
                std::cout << "heartbeat check ok." << std::endl;
            }
        }

        // 打印统计信息
        if( (now - last_statistic_time_) > 10*1000 ) 
        {
            unordered_map<TExchange, ExchangeStatistics> statistics;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_statistics_ };
                statistics = statistics_;
                for( auto& v : statistics_ ) {
                    v.second.reset();
                }
            }
            std::cout << "-------------" << std::endl;
            ExchangeStatistics total;
            for( const auto& v : statistics ) {
                std::cout << v.first << "\t" << v.second.get() << std::endl;
                total.accumlate(v.second);
            }
            std::cout << "total" << "\t" << total.get() << std::endl;
            std::cout << "-------------" << std::endl;
            last_statistic_time_ = now;
        }
    }
};

void RedisQuote::_update_statistics(const TExchange& exchange, const string& msg, const SDepthQuote& quote)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_statistics_ };
    statistics_[exchange].pkg_count += 1;
    statistics_[exchange].pkg_size += msg.length();
}
