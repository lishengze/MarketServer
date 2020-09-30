#include "redis_quote.h"
#include "stream_engine.h"
#include "stream_engine_config.h"

bool parse_snap(const string& data, SDepthQuote& quote, bool isSnap) {
    json snap_json = json::parse(data); 
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
    int count = 0;
    for (auto iter=snap_json[askDepth].begin(); count < MAX_DEPTH && iter!=snap_json[askDepth].end(); ++count, ++iter)
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        vassign(quote.Asks[count], price, volume);
    }
    quote.AskLength = count;

    count = 0;
    for (auto iter=snap_json[bidDepth].rbegin(); count < MAX_DEPTH && iter!=snap_json[bidDepth].rend(); ++count, ++iter)
    {
        const string& price = iter.key();
        const double& volume = iter.value();
        vassign(quote.Bids[count], price, volume);
    }
    quote.BidLength = count;
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
        if( !parse_snap(msg, quote, false))
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