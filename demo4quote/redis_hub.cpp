#include "redis_hub.h"
#include "stream_engine.h"

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


void RedisHub::OnMessage(const std::string& channel, const std::string& msg){
    if (channel.find(DEPTH_UPDATE_HEAD)!=string::npos)
    {
        int pos = channel.find(DEPTH_UPDATE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(DEPTH_UPDATE_HEAD), pos2-pos-strlen(DEPTH_UPDATE_HEAD));
        string exchange = channel.substr(pos2+1);
    
        if( symbol != "BTC_USDC" )
            return;  
        cout << "redis OnMessage:" << channel << " Msg: " << msg << "\n" << endl;

        SDepthQuote quote;
        if( !parse_snap(msg, quote, false))
            return;
        if( symbol != string(quote.Symbol) || exchange != string(quote.Exchange) ) {
            cout << "redis OnMessage: not match" << endl;
            return;
        }
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