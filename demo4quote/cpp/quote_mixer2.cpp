#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "quote_mixer.h"

std::shared_ptr<QuoteData> filter_quote(const string& symbol, const SMixQuote* src, const SDecimal& watermark)
{
    std::shared_ptr<QuoteData> msd = std::make_shared<QuoteData>();
    msd->set_symbol(symbol);

    // 卖盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = src->Asks;
        while( ptr != NULL && depth_count < CONFIG->grpc_push_depth_ ) {
            if( ptr->Price > watermark ) {
                DepthLevel* depth = msd->add_ask_depth();
                depth->mutable_price()->set_value(ptr->Price.Value);
                depth->mutable_price()->set_base(ptr->Price.Base);
                for(auto &v : ptr->Volume) {
                    const TExchange& exchange = v.first;
                    const double volume = v.second;
                    DepthVolume* depthVolume = depth->add_data();
                    depthVolume->set_volume(volume);
                    depthVolume->set_exchange(exchange);
                }
                depth_count += 1;
            }
            ptr = ptr->Next;
        }
    }
    // 买盘
    {
        int depth_count = 0;
        SMixDepthPrice* ptr = src->Bids;
        while( ptr != NULL && depth_count < CONFIG->grpc_push_depth_ ) {
            if( ptr->Price < watermark ) {
                DepthLevel* depth = msd->add_bid_depth();
                depth->mutable_price()->set_value(ptr->Price.Value);
                depth->mutable_price()->set_base(ptr->Price.Base);
                for(auto &v : ptr->Volume) {
                    const TExchange& exchange = v.first;
                    const double volume = v.second;
                    DepthVolume* depthVolume = depth->add_data();
                    depthVolume->set_volume(volume);
                    depthVolume->set_exchange(exchange);
                }
                depth_count += 1;
            }
            ptr = ptr->Next;
        }
    }

    return msd;
};

std::shared_ptr<QuoteData> QuoteMixer2::_publish_quote(const string& symbol, const SMixQuote* quote, bool isSnap) {
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
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/CONFIG->frequency_) )
    {
        return NULL;
    }
    // 如果watermark没有，也跳过
    SDecimal watermark;
    if( !_get_watermark(symbol, watermark) ) {
        return NULL;
    }
    last_clocks_[symbol] = get_miliseconds();
    return filter_quote(symbol, quote, watermark);
};

void QuoteMixer2::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    // compress price precise
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    SDepthQuote cpsQuote;
    compress_quote(symbol, quote, cpsQuote);
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
        snaps_[symbol][exchange] = quote;
    }

    std::shared_ptr<QuoteData> msd = NULL;
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) ) {
        ptr = new SMixQuote();
        symbols_[symbol] = ptr;
    } else {
        // 1. 清除老的exchange数据
        ptr->Asks = _clear_exchange(exchange, ptr->Asks);
        ptr->Bids = _clear_exchange(exchange, ptr->Bids);
    }
    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);

    // 4. 推送结果
    ptr->SequenceNo = cpsQuote.SequenceNo;
    msd = _publish_quote(symbol, ptr, true);

    // 执行发送
    if( msd != NULL ) {
        PUBLISHER->on_mix_snap2(symbol, msd);
    }
}

void QuoteMixer2::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    // compress price precise
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    SDepthQuote cpsQuote;
    compress_quote(symbol, quote, cpsQuote);

    std::shared_ptr<QuoteData> msd = NULL;
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) )
        return;
    // 1. 需要清除的价位数据
    ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, true);
    ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, false);

    // 3. 合并价位
    ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
    ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);

    // 4. 推送结果
    ptr->SequenceNo = cpsQuote.SequenceNo;
    msd = _publish_quote(symbol, ptr, true);

    // 执行发送
    if( msd != NULL ) {
        PUBLISHER->on_mix_snap2(symbol, msd);
    }
    return;
};

bool QuoteMixer2::_get_watermark(const string& symbol, SDecimal& watermark) const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };    
    auto wmIter = watermark_.find(symbol);
    if( wmIter == watermark_.end() || wmIter->second.Value == 0 ) {
        return false;
    }
    watermark = wmIter->second;
    return true;
};

void QuoteMixer2::_one_round() {

    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };
    // 计算watermark
    for( auto iter = snaps_.begin() ; iter != snaps_.end() ; ++iter ) {
        const TSymbol& symbol = iter->first;
        const unordered_map<TExchange, SDepthQuote>& snaps = iter->second;        
        vector<SDecimal> asks, bids;
        for( auto snapIter = snaps.begin() ; snapIter != snaps.end() ; ++snapIter ) {
            const SDepthQuote& quote = snapIter->second;
            if( quote.AskLength > 0 ) {
                asks.push_back(quote.Asks[0].Price);
            }
            if( quote.BidLength > 0  ) {
                bids.push_back(quote.Bids[0].Price);
            }
        }
        // 排序
        sort(asks.begin(), asks.end());
        sort(bids.begin(), bids.end());
        if( asks.size() > 0 && bids.size() > 0 ) {
            SDecimal watermark = (asks[asks.size()/2] + bids[bids.size()/2])/2;
            watermark_[symbol] = watermark;
        }
    }
}

void QuoteMixer2::_calc_watermark() {

    while( true ) {
        _one_round();

        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};
