#include "stream_engine_config.h"
#include "quote_mixer.h"

void compress_quote(const string& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    int precise = CONFIG->get_precise(symbol);
    
    vassign(dst.Exchange, src.Exchange);
    vassign(dst.Symbol, src.Symbol);
    vassign(dst.SequenceNo, src.SequenceNo);
    vassign(dst.TimeArrive, src.TimeArrive);

    // sell
    {
        int count = 0;
        SDecimal lastPrice;        
        for (int i = 0 ; count < MAX_DEPTH && i < src.AskLength; ++i )
        {
            const SDecimal& price = src.Asks[i].Price;
            const double& volume = src.Asks[i].Volume;
            SDecimal d;
            d.From(price, precise, true); // 卖价往上取整
            if( d > lastPrice ) {
                count ++;
                dst.Asks[count-1].Price = d;
                lastPrice = d;
            }
            dst.Asks[count-1].Volume += volume;
        }
        dst.AskLength = count;
    }
    // buy
    {
        int count = 0;
        SDecimal lastPrice;        
        for (int i = 0 ; count < MAX_DEPTH && i < src.BidLength; ++i )
        {
            const SDecimal& price = src.Bids[i].Price;
            const double& volume = src.Bids[i].Volume;
            SDecimal d;
            d.From(price, precise, false); // 卖价往上取整
            if( d > lastPrice ) {
                count ++;
                dst.Bids[count-1].Price = d;
                lastPrice = d;
            }
            dst.Bids[count-1].Volume += volume;
        }
        dst.BidLength = count;
    }
}

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
        _cross_askbid(ptr, quote);
    }
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, quote.Asks, quote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, quote.Bids, quote.BidLength, ptr->Watermark, false);

    // 4. 推送结果
    ptr->SequenceNo = quote.SequenceNo;
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
    SDepthQuote newQuote = quote;
    // 1. 需要清除的价位数据
    ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, newQuote.Asks, newQuote.AskLength, true);
    ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, newQuote.Bids, newQuote.BidLength, false);
    // 2. 修剪cross的价位
    _cross_askbid(ptr, newQuote);
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, newQuote.Asks, newQuote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, newQuote.Bids, newQuote.BidLength, ptr->Watermark, false);
    // 4. 推送结果
    ptr->SequenceNo = quote.SequenceNo;
    publish_quote(symbol, *ptr, false);
    return;
};
