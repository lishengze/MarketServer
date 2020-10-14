#include "redis_quote.h"
#include "stream_engine.h"
#include "stream_engine_config.h"

bool parse_quote(const string& data, SDepthQuote& quote, bool isSnap) {
    njson snap_json = njson::parse(data); 
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq"].get<long long>(); 
    
    vassign(quote.Exchange, exchange);
    vassign(quote.Symbol, symbol);
    vassign(quote.SequenceNo, sequence_no);
    vassign(quote.TimeArrive, timeArrive);
    
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";

    // sell
    {
        map<SDecimal, double> depths;
        for (auto iter = snap_json[askDepth].begin() ; iter != snap_json[askDepth].end() ; ++iter )
        {
            const string& price = iter.key();
            const double& volume = iter.value();
            SDecimal dPrice;
            dPrice.From(price, -1);
            depths[dPrice] = volume;
        }

        int count = 0;
        for( auto iter = depths.begin() ; count < MAX_DEPTH && iter != depths.end() ; ++iter, ++count )
        {
            quote.Asks[count].Price = iter->first;
            quote.Asks[count].Volume = iter->second;
        }
        quote.AskLength = count;
    }

    // buy
    {
        map<SDecimal, double> depths;
        for (auto iter = snap_json[bidDepth].begin() ; iter != snap_json[bidDepth].end() ; ++iter )
        {
            const string& price = iter.key();
            const double& volume = iter.value();
            SDecimal dPrice;
            dPrice.From(price, -1);
            depths[dPrice] = volume;
        }

        int count = 0;
        for( auto iter = depths.rbegin() ; count < MAX_DEPTH && iter != depths.rend() ; ++iter, ++count )
        {
            quote.Bids[count].Price = iter->first;
            quote.Bids[count].Volume = iter->second;
        }
        quote.BidLength = count;
    }
    return true;
}


void RedisQuote::start(const string& host, const int& port, const string& password, UTLogPtr logger) {
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(host, port, password, utrade::pandora::RM_Subscribe);
    
    // request snap 
    redis_snap_requester_.init(host, port, password, logger);
    redis_snap_requester_.set_engine(this);
    redis_snap_requester_.start();
};

void RedisQuote::_on_snap(const string& exchange, const string& symbol, const string& data) {   
    if( exchange == "HUOBI" && symbol == "BTC_USDT") {
        cout << "redis _on_snap:" << data << endl;
    }

    if( CONFIG->output_to_screen_ )
        cout << "redis OnSnap:" << symbol << " Msg: " << data << endl;

    // string ->  SDepthQuote
    SDepthQuote quote;
    if( !parse_quote(data, quote, true))
        return;        
    if( symbol != string(quote.Symbol) || exchange != string(quote.Exchange) ) {
        UT_LOG_ERROR(CONFIG->logger_, "get_snap: not match");
        return;
    }

    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    markets_[exchange][symbol] = quote;
    engine_interface_->on_snap(exchange, symbol, quote);
};

void RedisQuote::OnMessage(const std::string& channel, const std::string& msg){
    if (channel.find(DEPTH_UPDATE_HEAD)!=string::npos)
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
                cout << "redis OnMessage:" << channel << " Msg: " << msg << endl;
            }
        }

        // string -> SDepthQuote
        SDepthQuote quote;
        if( !parse_quote(msg, quote, false))
            return;
        if( symbol != string(quote.Symbol) || exchange != string(quote.Exchange) ) {
            UT_LOG_ERROR(CONFIG->logger_, "redis OnMessage: not match");
            return;
        }

        // redis_snap_requester_ add symbol
        redis_snap_requester_.on_update_symbol(exchange, symbol);

        // safe callback update
        std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
        SDepthQuote lastQuote;
        if( !_get_quote(exchange, symbol, lastQuote) )
            return;
        // filter by SequenceNo
        if( quote.SequenceNo < lastQuote.SequenceNo )
            return;
        engine_interface_->on_update(exchange, symbol, quote);
    }
    else if(channel.find(TICK_HEAD)!=string::npos)
    {
    }
    else
    {
        //UT_LOG_WARNING(logger_, "Unknown Message Type");
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
