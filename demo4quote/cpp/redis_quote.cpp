#include "redis_quote.h"
#include "stream_engine.h"
#include "stream_engine_config.h"

bool parse_snap(const string& data, SDepthQuote& quote, bool isSnap, int precise) {
    json snap_json = json::parse(data); 
    string symbol = snap_json["Symbol"].get<std::string>();
    string exchange = snap_json["Exchange"].get<std::string>();
    string timeArrive = snap_json["TimeArrive"].get<std::string>();
    long long sequence_no = snap_json["Msg_seq"].get<long long>(); 
    
    vassign(quote.Exchange, exchange);
    vassign(quote.Symbol, symbol);
    vassign(quote.SequenceNo, sequence_no);
    vassign(quote.TimeArrive, timeArrive);
    
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    string askDepth = isSnap ? "AskDepth" : "AskUpdate";
    string bidDepth = isSnap ? "BidDepth" : "BidUpdate";
    {
        int count = 0;
        SDecimal lastPrice;
        for (auto iter=snap_json[askDepth].begin(); count < MAX_DEPTH && iter!=snap_json[askDepth].end(); ++iter)
        {
            const string& price = iter.key();
            const double& volume = iter.value();
            SDecimal d;
            d.From(price, precise, true); // 卖价往上取整
            if( d > lastPrice ) {
                count ++;
                quote.Asks[count-1].Price = d;
                lastPrice = d;
            }
            quote.Asks[count-1].Volume += volume;
        }
        quote.AskLength = count;
        //cout << "ask " << count << endl;
    }

    {
        int count = 0;
        SDecimal lastPrice;
        lastPrice.Value = 9999999999;
        for (auto iter=snap_json[bidDepth].rbegin(); count < MAX_DEPTH && iter!=snap_json[bidDepth].rend(); ++iter)
        {
            const string& price = iter.key();
            const double& volume = iter.value();
            SDecimal d;
            d.From(price, precise); // 买价往下取整
            if( d < lastPrice ) {
                count ++;
                quote.Bids[count-1].Price = d;
                lastPrice = d;
            }
            quote.Bids[count-1].Volume += volume;
        }
        quote.BidLength = count;
        //cout << "bid " << count << endl;
    }
    return true;
}


void RedisQuote::OnMessage(const std::string& channel, const std::string& msg){
    if (channel.find(DEPTH_UPDATE_HEAD)!=string::npos)
    {
        int pos = channel.find(DEPTH_UPDATE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(DEPTH_UPDATE_HEAD), pos2-pos-strlen(DEPTH_UPDATE_HEAD));
        string exchange = channel.substr(pos2+1);
    
        if( CONFIG->sample_symbol_ != "" && symbol != CONFIG->sample_symbol_ )
            return;  
        UT_LOG_INFO(CONFIG->logger_, "redis OnMessage:" << channel << " Msg: " << msg);
        if( CONFIG->output_to_screen_ )
            cout << "redis OnMessage:" << channel << " Msg: " << msg << endl;

        SDepthQuote quote;
        if( !parse_snap(msg, quote, false, CONFIG->get_precise(symbol)))
            return;
        if( symbol != string(quote.Symbol) || exchange != string(quote.Exchange) ) {
            UT_LOG_ERROR(CONFIG->logger_, "redis OnMessage: not match");
            return;
        }
        redis_snap_.on_update_symbol(exchange, symbol);
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
    //redis_api_->PSubscribeTopic("*");
    //redis_api_->PSubscribeTopic("UPDATEx|*.OKEX");
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
            const string& symbol = *iterSymbol;
            const string& exchange = *iterExchange;
            redis_api_->SubscribeTopic("UPDATEx|" + symbol + "." + exchange);            
        }
    }
};