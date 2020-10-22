#include "stream_engine_config.h"
#include "quote_mixer.h"
#include "converter.h"

void QuoteMixer::publish_quote(const string& symbol, const SMixQuote& quote, bool isSnap) {
    // 每秒更新频率控制
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/CONFIG->grpc_publish_frequency_) )
    {
        return;
    }
    last_clocks_[symbol] = get_miliseconds();

    // 发送
    std::shared_ptr<QuoteData> ptr = mixquote_to_pbquote("", symbol, quote);
    PUBLISHER->on_mix_snap(symbol, ptr);
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
        ptr->asks = _clear_exchange(exchange, ptr->asks);
        ptr->bids = _clear_exchange(exchange, ptr->bids);
        // 2. 修剪cross的价位
        _cross_askbid(ptr, cpsQuote);
    }
    // 3. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, ptr->watermark, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, ptr->watermark, false);

    // 4. 推送结果
    ptr->sequence_no = cpsQuote.sequence_no;
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
    ptr->asks = _clear_pricelevel(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, true);
    ptr->bids = _clear_pricelevel(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, false);
    // 2. 修剪cross的价位
    _cross_askbid(ptr, cpsQuote);
    // 3. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, ptr->watermark, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, ptr->watermark, false);
    // 4. 推送结果
    ptr->sequence_no = cpsQuote.sequence_no;
    publish_quote(symbol, *ptr, false);
    return;
};

bool QuoteMixer::_get_quote(const string& symbol, SMixQuote*& ptr) const {
    auto iter = symbols_.find(symbol);
    if( iter == symbols_.end() )
        return false;
    ptr = iter->second;
    return true;
}

SMixDepthPrice* QuoteMixer::_clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;

    for( int i = 0 ; i < newLength ; ++i ) {
        const SDepthPrice& depth = newDepths[i];
        if( depth.volume > VOLUME_PRECISE ) {
            continue;
        }
        // 找到并删除对应价位
        while( tmp != NULL ) {
            if( tmp->price == depth.price ) {                
                unordered_map<TExchange, double>& volumeByExchange = tmp->volume;
                auto iter = volumeByExchange.find(exchange);
                if( iter != volumeByExchange.end() ) {
                    volumeByExchange.erase(iter);                
                    if( volumeByExchange.size() == 0 ) {
                        // 删除             
                        SMixDepthPrice* waitToDel = tmp;
                        last->next = tmp->next;
                        tmp = tmp->next;
                        delete waitToDel;
                        break;
                    }
                }
            } else if ( isAsk ? (tmp->price > depth.price) : (tmp->price < depth.price) ) {
                break;
            }
            last = tmp;
            tmp = tmp->next;
        }
    }
    return head.next;
}

SMixDepthPrice* QuoteMixer::_clear_exchange(const string& exchange, SMixDepthPrice* depths) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;
    while( tmp != NULL ) {
        unordered_map<TExchange, double>& volumeByExchange = tmp->volume;
        auto iter = volumeByExchange.find(exchange);
        if( iter != volumeByExchange.end() ) {
            volumeByExchange.erase(iter);                
            if( volumeByExchange.size() == 0 ) {
                // 删除             
                SMixDepthPrice* waitToDel = tmp;
                last->next = tmp->next;
                tmp = tmp->next;
                delete waitToDel;
                continue;
            }
        }
        last = tmp;
        tmp = tmp->next;
    }
    return head.next;
}

void QuoteMixer::_cross_askbid(SMixQuote* mixedQuote, const SDepthQuote& quote) {
    // 新行情的买1价 大于 聚合行情的卖1价
    if( mixedQuote->asks != NULL && quote.bid_length > 0 && mixedQuote->asks->price < quote.bids[0].price ) {
        mixedQuote->watermark = (quote.bids[0].price + mixedQuote->asks->price) / 2;
    }
    // 新行情的卖1价 小于 聚合行情的买1价
    else if( mixedQuote->bids != NULL && quote.ask_length > 0 && mixedQuote->bids->price > quote.asks[0].price ) {
        mixedQuote->watermark = (mixedQuote->bids->price + quote.asks[0].price) / 2;
    }
    else {
        mixedQuote->watermark = SDecimal();
    }
}

SMixDepthPrice* QuoteMixer::_mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
    const int& length, const SDecimal& watermark, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;

    // 1. 裁剪聚合行情对应价位（开区间）
    SMixDepthPrice* tmp = mixedDepths;
    while( tmp != NULL ) {
        if( watermark.value == 0 || (isAsk ? (tmp->price > watermark) : (tmp->price < watermark)) ) {
            break;
        } else {
            SMixDepthPrice* waitToDel = tmp;
            // 删除
            last->next = tmp->next;       
            tmp = tmp->next;
            delete waitToDel;
        }
    }

    // 2. 裁剪新行情对应价位（闭区间）
    int filteredLevel = 99999999;
    double virtualVolume = 0; // 虚拟价位挂单量
    for( int i = 0 ; i < length ; ++ i ) {
        const SDepthPrice& depth = depths[i];
        if( depth.volume < VOLUME_PRECISE ) {
            continue;
        }
        if( watermark.value == 0 || (isAsk ? (depth.price >= watermark) : (depth.price <= watermark)) ) {
            filteredLevel = i;
            break;        
        } else {
            virtualVolume += depth.volume;
        }
    }

    // 3. 插入虚拟价位
    if( virtualVolume > 0 && watermark.value > 0 ) {
        SMixDepthPrice* virtualDepth = new SMixDepthPrice();
        virtualDepth->price = watermark;
        virtualDepth->volume[exchange] = virtualVolume;
        // 加入链表头部
        virtualDepth->next = head.next;
        head.next = virtualDepth;
    }

    // 4. 混合：first + depths
    int i = filteredLevel;
    for( tmp = head.next ; i < length && tmp != NULL ; ) {
        const SDepthPrice& depth = depths[i];
        if( depth.volume < VOLUME_PRECISE ) {
            ++i;
            continue;
        }
        if( isAsk ? (depth.price < tmp->price) : (depth.price > tmp->price) ) {
            // 新建价位
            SMixDepthPrice *newDepth = new SMixDepthPrice();
            newDepth->price = depth.price;
            newDepth->volume[exchange] = depth.volume;
            
            last->next = newDepth;
            last = newDepth;
            newDepth->next = tmp;
            ++i;
        } else if( depth.price == tmp->price ) {
            tmp->volume[exchange] = depth.volume;
            ++i;
        } else {
            last = tmp;
            tmp = tmp->next;
        }
    }

    // 5. 剩余全部加入队尾
    for( ; i < length ; ++i) {
        const SDepthPrice& depth = depths[i];
        if( depth.volume < VOLUME_PRECISE ) {
            continue;
        }
        // 新建价位
        SMixDepthPrice *newDepth = new SMixDepthPrice();
        newDepth->price = depth.price;
        newDepth->volume[exchange] = depth.volume;
        newDepth->next = NULL;
        // 插入
        last->next = newDepth;
        last = newDepth;
    }

    // 6. 删除多余的价位
    tmp = head.next;
    int count = 0;
    while( tmp != NULL && count < MAX_MIXDEPTH) {
        tmp = tmp->next;
        count ++;
    }
    if( tmp != NULL ) {
        SMixDepthPrice* deletePtr = tmp->next;
        tmp->next = NULL;
        while( deletePtr != NULL ) {
            SMixDepthPrice *waitToDelete = deletePtr;
            deletePtr = deletePtr->next;
            delete waitToDelete;
        }
    }

    return head.next;
};
