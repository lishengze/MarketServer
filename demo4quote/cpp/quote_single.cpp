#include "stream_engine_config.h"
#include "quote_single.h"
#include "grpc_server.h"

void QuoteSingle::publish_quote(const string& exchange, const string& symbol, const SMixQuote& quote, bool isSnap) {
    /*if( quote.Asks != NULL ) {
        cout << quote.Asks->Price.GetValue();
    } else {    
        cout << "-";
    }
    cout << ",";
    if( quote.Bids != NULL ) {
        cout << quote.Bids->Price.GetValue();
    } else {
        cout << "-";
    }
    cout << endl;*/

    // 每秒更新频率控制
    auto last = last_clocks_[exchange][symbol];
    auto now = get_miliseconds();
    if( (now -last) < (1000/CONFIG->frequency_) )
    {
        return;
    }
    last_clocks_[exchange][symbol] = now;
    PUBLISHER->on_snap(exchange, symbol, quote);
};

void QuoteSingle::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    SMixQuote* ptr = NULL;
    if( !_get_quote(exchange, symbol, ptr) ) {
        ptr = new SMixQuote();
        symbols_[exchange][symbol] = ptr;
    } else {
        ptr->Asks = _clear_allpricelevel(exchange, ptr->Asks);
        ptr->Bids = _clear_allpricelevel(exchange, ptr->Bids);
    }
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, quote.Asks, quote.AskLength, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, quote.Bids, quote.BidLength, false);

    // 4. 推送结果
    ptr->SequenceNo = quote.SequenceNo;
    publish_quote(exchange, symbol, *ptr, true);
    return;
}

void QuoteSingle::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    SMixQuote* ptr = NULL;
    if( !_get_quote(exchange, symbol, ptr) )
        return;
    // 1. 需要清除的价位数据
    ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, quote.Asks, quote.AskLength, true);
    ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, quote.Bids, quote.BidLength, false);
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, quote.Asks, quote.AskLength, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, quote.Bids, quote.BidLength, false);
    // 4. 推送结果
    ptr->SequenceNo = quote.SequenceNo;
    publish_quote(exchange, symbol, *ptr, false);
    return;
};
