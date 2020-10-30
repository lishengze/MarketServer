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
    long long sequence_no = snap_json["Msg_seq"].get<long long>(); 
    
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


void RedisQuote::start(const RedisParams& params, UTLogPtr logger) {
    // 请求增量
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(params.host, params.port, params.password, utrade::pandora::RM_Subscribe);
    
    // 请求全量
    redis_snap_requester_.init(params, logger);
    redis_snap_requester_.set_engine(this);
    redis_snap_requester_.start();
};

void RedisQuote::_on_snap(const TExchange& exchange, const TSymbol& symbol, const string& data) {   
    //if( exchange == "HUOBI" && symbol == "BTC_USDT") {
    //    cout << "redis snap " << exchange << "." << symbol << ":" << data << endl;
    //}

    // log snap message
    if( CONFIG->output_to_screen_ ) {
        if( CONFIG->sample_symbol_ == "" || symbol == CONFIG->sample_symbol_ ) {
            cout << "redis snap " << exchange << "." << symbol << ":" << data << endl;
        }
    }

    // string ->  SDepthQuote
    SDepthQuote quote;
    if( !redisquote_to_quote(data, quote, true))
        return;        
    if( symbol != string(quote.symbol) || exchange != string(quote.exchange) ) {
        UT_LOG_ERROR(CONFIG->logger_, "get_snap: not match");
        return;
    }

    // 保存快照
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
        markets_[exchange][symbol] = quote;
    }

    // 回调
    engine_interface_->on_snap(exchange, symbol, quote);
};

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg){
    if (channel.find(DEPTH_UPDATE_HEAD) != string::npos)
    {
        // decode exchange and symbol
        int pos = channel.find(DEPTH_UPDATE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(DEPTH_UPDATE_HEAD), pos2-pos-strlen(DEPTH_UPDATE_HEAD));
        string exchange = channel.substr(pos2+1);

        //if( exchange == "HUOBI" && symbol == "BTC_USDT") {
        //    cout << "redis OnMessage:" << channel << " Msg: " << msg << endl;
        //}
        
        // log update message
        UT_LOG_INFO(CONFIG->logger_, "redis OnMessage:" << channel << " Msg: " << msg);
        if( CONFIG->output_to_screen_ ) {
            if( CONFIG->sample_symbol_ == "" || symbol == CONFIG->sample_symbol_ ) {
                cout << "redis update " << exchange << "." << symbol << ":" << msg << endl;
            }
        }

        // redis结构到内部结构的转换
        SDepthQuote quote;
        if( !redisquote_to_quote(msg, quote, false)) {
            UT_LOG_ERROR(CONFIG->logger_, "redis OnMessage: redisquote_to_quote failed");
            return;
        }
        if( symbol != string(quote.symbol) || exchange != string(quote.exchange) ) {
            UT_LOG_ERROR(CONFIG->logger_, "redis OnMessage: not match");
            return;
        }

        // 添加到全量请求任务中
        redis_snap_requester_.add_symbol(exchange, symbol);

        // 检查快照和增量间的序号
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
            SDepthQuote lastQuote;
            if( !_get_quote(exchange, symbol, lastQuote) ) {
                std::cout << "_get_quote failed" << std::endl;
                return;
            }
            if( quote.sequence_no < lastQuote.sequence_no ) {
                std::cout << "sequence_no check failed:" << exchange << ":" << symbol << " " << quote.sequence_no << "/" << lastQuote.sequence_no << std::endl;
                return;
            }
        }

        // 回调
        //std::cout << "on_update " << exchange << " " << symbol << std::endl;
        engine_interface_->on_update(exchange, symbol, quote);
    }
    else if(channel.find(TICK_HEAD)!=string::npos)
    {
    }
    else
    {
        UT_LOG_ERROR(CONFIG->logger_, "Unknown Message Type");
    }
};

void RedisQuote::OnConnected() {
    cout << "\n##### Redis MarketDispatcher::OnConnected ####\n" << endl;
    engine_interface_->on_connected();
};

bool RedisQuote::_get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) const {
    auto iter = markets_.find(exchange);
    if( iter == markets_.end() )
        return false;
    const TMarketQuote& marketQuote = iter->second;
    auto iter2 = marketQuote.find(symbol);
    if( iter2 == marketQuote.end() )
        return false;
    quote = iter2->second;
    return true;
};

void RedisQuote::subscribe(const string& channel) {
    redis_api_->SubscribeTopic(channel);
};
