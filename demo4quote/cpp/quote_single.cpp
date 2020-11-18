#include "stream_engine_config.h"
#include "quote_single.h"
#include "converter.h"

void QuoteSingle::clear_exchange(const TExchange& exchange)
{
    // 需要刷新的全量
    unordered_map<TSymbol, std::shared_ptr<MarketStreamData>> snaps;

    // 生成所有该交易所品种的空包
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        auto iter = quotes_.find(exchange);
        if( iter != quotes_.end() ) {
            for( const auto& v : iter->second ) {
                SMixQuote empty;
                empty.sequence_no = v.second->sequence_no + 1;
                snaps[v.first] = mixquote_to_pbquote2(exchange, v.first, &empty, true);
            }
            quotes_.erase(iter);
        }
    }

    // 发送空包更新下游
    for( auto& v : snaps ) {
        PUBLISHER->publish_single(exchange, v.first, v.second, NULL);
    }
}

void QuoteSingle::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    std::shared_ptr<MarketStreamData> pub_snap;
    if( !_on_snap(exchange, symbol, quote, pub_snap) )
        return;
    PUBLISHER->publish_single(exchange, symbol, pub_snap, NULL);
}

void QuoteSingle::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    std::shared_ptr<MarketStreamData> pub_snap, pub_diff;
    if( !_on_update(exchange, symbol, quote, pub_snap, pub_diff) )
        return;
    PUBLISHER->publish_single(exchange, symbol, pub_snap, pub_diff);
}

bool QuoteSingle::_on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(exchange, symbol, ptr) ) {
        ptr = new SMixQuote();
        quotes_[exchange][symbol] = ptr;
    } else {
        ptr->release();
    }
    // 合并价位
    ptr->sequence_no = quote.sequence_no;
    ptr->asks = _mix_exchange(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, quote.bids, quote.bid_length, false);

    // 发送
    pub_snap = mixquote_to_pbquote2(exchange, symbol, ptr, true);
    return true;
}

bool QuoteSingle::_on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(exchange, symbol, ptr) )
        return false;    
    // 合并价位
    ptr->asks = _clear_pricelevel(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->asks = _mix_exchange(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->bids = _clear_pricelevel(exchange, ptr->bids, quote.bids, quote.bid_length, false);
    ptr->bids = _mix_exchange(exchange, ptr->bids, quote.bids, quote.bid_length, false);
    ptr->sequence_no = quote.sequence_no;

    // 发送
    pub_snap = mixquote_to_pbquote2(exchange, symbol, ptr, true);
    pub_diff = depth_to_pbquote2(exchange, symbol, quote);
    return true;
}

bool QuoteSingle::_get_quote(const TExchange& exchange, const TSymbol& symbol, SMixQuote*& ptr) const {
    auto iter = quotes_.find(exchange);
    if( iter == quotes_.end() )
        return false;
    auto iter2 = iter->second.find(symbol);
    if( iter2 == iter->second.end() )
        return false;        
    ptr = iter2->second;
    return true;
}

SMixDepthPrice* QuoteSingle::_clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
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
                // 删除             
                SMixDepthPrice* waitToDel = tmp;
                last->next = tmp->next;
                tmp = tmp->next;
                delete waitToDel;
                break;
            } else if ( isAsk ? (tmp->price > depth.price) : (tmp->price < depth.price) ) {
                break;
            }
            last = tmp;
            tmp = tmp->next;
        }
    }
    return head.next;
}

/*
SMixDepthPrice* QuoteSingle::_update_exchange(const TExchange& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
    SMixDepthPrice head;
    head.next = depths;        
    SMixDepthPrice *tmp = depths, *last = &head;

    for( int i = 0 ; i < newLength ;) {
        const SDepthPrice& depth = newDepths[i];
        if( tmp == NULL || (isAsk ? (depth.price < tmp->price) : (depth.price > tmp->price)) ) {
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
    return head.next;
}*/

SMixDepthPrice* QuoteSingle::_mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
    const int& length, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;

    // 1. 混合
    int i = 0;
    SMixDepthPrice* tmp = mixedDepths;
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

    // 2. 剩余全部加入队尾
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

    // 3. 删除多余的价位
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
}
