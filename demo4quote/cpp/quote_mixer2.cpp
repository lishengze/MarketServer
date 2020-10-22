#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "quote_mixer.h"
#include "converter.h"

QuoteMixer2::QuoteMixer2() {
    thread_loop_ = new std::thread(&QuoteMixer2::_calc_watermark, this);
}   

QuoteMixer2::~QuoteMixer2() {
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}

void QuoteMixer2::_publish_quote(const string& symbol, const SMixQuote* quote, bool isSnap) {
    // 每秒更新频率控制
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/CONFIG->grpc_publish_frequency_) )
    {
        return;
    }
    // 如果watermark没有，也跳过
    SDecimal watermark;
    if( !_get_watermark(symbol, watermark) ) {
        return;
    }
    last_clocks_[symbol] = get_miliseconds();

    std::shared_ptr<QuoteData> ptr = mixquote_to_pbquote2(symbol, quote, watermark);
    PUBLISHER->on_mix_snap(symbol, ptr);
};

void QuoteMixer2::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
    // compress price precise
    // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
    SDepthQuote cpsQuote;
    compress_quote(symbol, quote, cpsQuote);
    std::cout << "after compress " << symbol << " " << cpsQuote.ask_length << "/" << cpsQuote.bid_length << std::endl;
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
        ptr->asks = _clear_exchange(exchange, ptr->asks);
        ptr->bids = _clear_exchange(exchange, ptr->bids);
    }
    // 3. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, false);

    ptr->sequence_no = cpsQuote.sequence_no;

    // 4. 推送结果
    std::cout << "publish " << symbol << " " << ptr->ask_length() << "/" << ptr->bid_length() << std::endl;
    _publish_quote(symbol, ptr, true);
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
    ptr->asks = _clear_pricelevel(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, true);
    ptr->bids = _clear_pricelevel(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, false);

    // 3. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, cpsQuote.asks, cpsQuote.ask_length, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, cpsQuote.bids, cpsQuote.bid_length, false);

    ptr->sequence_no = cpsQuote.sequence_no;

    // 4. 推送结果
    _publish_quote(symbol, ptr, true);
    return;
};

bool QuoteMixer2::_get_watermark(const string& symbol, SDecimal& watermark) const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_snaps_ };    
    auto wmIter = watermark_.find(symbol);
    if( wmIter == watermark_.end() || wmIter->second.value == 0 ) {
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
            if( quote.ask_length > 0 ) {
                asks.push_back(quote.asks[0].price);
            }
            if( quote.bid_length > 0  ) {
                bids.push_back(quote.bids[0].price);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

bool QuoteMixer2::_get_quote(const string& symbol, SMixQuote*& ptr) const {
    auto iter = symbols_.find(symbol);
    if( iter == symbols_.end() )
        return false;
    ptr = iter->second;
    return true;
}

SMixDepthPrice* QuoteMixer2::_clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
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

SMixDepthPrice* QuoteMixer2::_clear_exchange(const string& exchange, SMixDepthPrice* depths) {
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

SMixDepthPrice* QuoteMixer2::_mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
    const int& length, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;
    SMixDepthPrice* tmp = mixedDepths;

    // 4. 混合：first + depths
    int i = 0;
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
