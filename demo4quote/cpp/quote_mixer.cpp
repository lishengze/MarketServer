#include "stream_engine_config.h"
#include "quote_mixer.h"
#include "grpc_server.h"

void compress_quote(const string& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    int precise = CONFIG->get_precise(symbol);
    
    vassign(dst.Exchange, src.Exchange);
    vassign(dst.Symbol, src.Symbol);
    vassign(dst.SequenceNo, src.SequenceNo);
    vassign(dst.TimeArrive, src.TimeArrive);

    // sell
    {
        int count = 0;
        SDecimal lastPrice = SDecimal::MinDecimal();
        for (int i = 0 ; count < MAX_DEPTH && i < src.AskLength; ++i )
        {
            const SDecimal& price = src.Asks[i].Price;
            const double& volume = src.Asks[i].Volume;

            // 卖价往上取整
            SDecimal scaledPrice;
            scaledPrice.From(price, precise, true); 

            if( scaledPrice > lastPrice ) {
                count ++;
                dst.Asks[count-1].Price = scaledPrice;
                lastPrice = scaledPrice;
            }
            dst.Asks[count-1].Volume += volume;
        }
        dst.AskLength = count;
    }
    // buy
    {
        int count = 0;
        SDecimal lastPrice = SDecimal::MaxDecimal();
        for (int i = 0 ; count < MAX_DEPTH && i < src.BidLength; ++i )
        {
            const SDecimal& price = src.Bids[i].Price;
            const double& volume = src.Bids[i].Volume;
            
            // 买价往下取整
            SDecimal scalePrice;
            scalePrice.From(price, precise, false); 

            if( scalePrice < lastPrice ) {
                count ++;
                dst.Bids[count-1].Price = scalePrice;
                lastPrice = scalePrice;
            }
            dst.Bids[count-1].Volume += volume;
        }
        dst.BidLength = count;
    }
}

void QuoteMixer::publish_quote(const string& symbol, const SMixQuote& quote, bool isSnap) {
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
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/CONFIG->grpc_publish_frequency_) )
    {
        return;
    }
    last_clocks_[symbol] = get_miliseconds();
    PUBLISHER->on_mix_snap(symbol, quote);
};

void QuoteMixer::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    // compress price precise
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    SDepthQuote cpsQuote;
    compress_quote(symbol, quote, cpsQuote);

    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) ) {
        ptr = new SMixQuote();
        symbols_[symbol] = ptr;
    } else {
        // 1. 清除老的exchange数据
        ptr->Asks = _clear_exchange(exchange, ptr->Asks);
        ptr->Bids = _clear_exchange(exchange, ptr->Bids);
        // 2. 修剪cross的价位
        _cross_askbid(ptr, cpsQuote);
    }
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);

    // 4. 推送结果
    ptr->SequenceNo = cpsQuote.SequenceNo;
    publish_quote(symbol, *ptr, true);
    return;
}

void QuoteMixer::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    // compress price precise
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    SDepthQuote cpsQuote;
    compress_quote(symbol, quote, cpsQuote);

    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) )
        return;
    // 1. 需要清除的价位数据
    ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, true);
    ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, false);
    // 2. 修剪cross的价位
    _cross_askbid(ptr, cpsQuote);
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);
    // 4. 推送结果
    ptr->SequenceNo = cpsQuote.SequenceNo;
    publish_quote(symbol, *ptr, false);
    return;
};
