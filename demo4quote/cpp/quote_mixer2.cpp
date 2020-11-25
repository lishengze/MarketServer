#include "stream_engine_config.h"
#include "quote_mixer2.h"
#include "converter.h"

/////////////////////////////////////////////////////////////////////
QuoteMixer2::QuoteMixer2() {
}   

QuoteMixer2::~QuoteMixer2() {
}

bool QuoteMixer2::_check_update_clocks(const TSymbol& symbol) {
    if( CONFIG->grpc_publish_frequency_ == 0 )
        return true;

    std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
    // 每秒更新频率控制
    auto iter = last_clocks_.find(symbol);
    if( iter != last_clocks_.end() && (get_miliseconds() -iter->second) < (1000/CONFIG->grpc_publish_frequency_) )
    {
        return false;
    }
    last_clocks_[symbol] = get_miliseconds();
    return true;
}

void QuoteMixer2::_publish_quote(const TSymbol& symbol, std::shared_ptr<MarketStreamData> pub_snap, std::shared_ptr<MarketStreamData> pub_diff, bool is_snap) 
{
    if( is_snap ) {
        std::cout << "publish(snap) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    } else {
        std::cout << "publish(update) " << symbol << " " << pub_snap->asks_size() << "/" << pub_snap->bids_size() << std::endl;
    }
    PUBLISHER->publish_mix(symbol, pub_snap, pub_diff);
}

void QuoteMixer2::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) {
    // 预处理
    SDepthQuote cpsQuote;
    _preprocess(exchange, symbol, quote, cpsQuote);

    // 更新内存中的行情
    std::shared_ptr<MarketStreamData> pub_snap;
    if( !_on_snap(exchange, symbol, cpsQuote, pub_snap) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, NULL, true);
}

void QuoteMixer2::change_precise(const TSymbol& symbol, int precise)
{
    std::shared_ptr<MarketStreamData> pub_snap;

    // 清除行情数据
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        auto iter = quotes_.find(symbol);
        if( iter != quotes_.end() )
        {
            iter->second->release();
            pub_snap = mixquote_to_pbquote2("", symbol, iter->second, true);
        } else {
            SMixQuote tmp;
            pub_snap = mixquote_to_pbquote2("", symbol, &tmp, true);
        }
    }

    // 清除频率记号
    {        
        std::unique_lock<std::mutex> inner_lock{ mutex_clocks_ };
        auto iter = last_clocks_.find(symbol);
        if( iter != last_clocks_.end() ) {
            last_clocks_.erase(iter);
        }
    }

    // 广播snap
    _publish_quote(symbol, pub_snap, NULL, true);
}

void QuoteMixer2::clear_exchange(const TExchange& exchange)
{    
    unordered_map<TSymbol, std::shared_ptr<MarketStreamData>> snaps;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
        for( auto&v : quotes_ ) {
            //_println_("QuoteMixer2 clear exchange %s: %s...", exchange.c_str(), v.first.c_str());
            v.second->asks = _clear_exchange(exchange, v.second->asks);
            v.second->bids = _clear_exchange(exchange, v.second->bids);
            //_println_("QuoteMixer2 clear exchange %s: %s after clear ...", exchange.c_str(), v.first.c_str());
            snaps[v.first] = mixquote_to_pbquote2("", v.first, v.second, true);
            //_println_("QuoteMixer2 clear exchange %s: %s", exchange.c_str(), v.first.c_str());
        }
    }

    for( auto& v : snaps ) {
        _publish_quote(v.first, v.second, NULL, true);
    }
}

void QuoteMixer2::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    // 预处理
    SDepthQuote cpsQuote;
    _preprocess(exchange, symbol, quote, cpsQuote);

    // 更新内存中的行情
    std::shared_ptr<MarketStreamData> pub_snap, pub_diff;
    if( !_on_update(exchange, symbol, cpsQuote, pub_snap, pub_diff) )
        return;

    // 推送结果
    _publish_quote(symbol, pub_snap, pub_diff, false);
}

bool QuoteMixer2::_on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) ) {
        ptr = new SMixQuote();
        quotes_[symbol] = ptr;
    } else {
        // 1. 清除老的exchange数据
        ptr->asks = _clear_exchange(exchange, ptr->asks);
        ptr->bids = _clear_exchange(exchange, ptr->bids);
    }
    // 2. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, quote.bids, quote.bid_length, false);
    ptr->sequence_no = quote.sequence_no;
    
    // 检查发布频率
    //if( symbol == "BTC_USDT" ) {
    //    std::cout << quote.exchange << " " << quote.ask_length << " " << quote.asks[0].price.get_str_value() << " " << 
    //    ptr->asks->price.get_str_value() << " " << ptr->asks->next->price.get_str_value() << " " << ptr->asks->next->next->price.get_str_value() << std::endl;
    //}
    if( !_check_update_clocks(symbol) ) {
        return false;
    }

    pub_snap = mixquote_to_pbquote2("", symbol, ptr, true);
    return true;
}

bool QuoteMixer2::_on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote, std::shared_ptr<MarketStreamData>& pub_snap, std::shared_ptr<MarketStreamData>& pub_diff)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_quotes_ };
    SMixQuote* ptr = NULL;
    if( !_get_quote(symbol, ptr) )
        return false;

    // 1. 需要清除的价位数据
    ptr->asks = _clear_pricelevel(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->bids = _clear_pricelevel(exchange, ptr->bids, quote.bids, quote.bid_length, false);

    // 2. 合并价位
    ptr->asks = _mix_exchange(exchange, ptr->asks, quote.asks, quote.ask_length, true);
    ptr->bids = _mix_exchange(exchange, ptr->bids, quote.bids, quote.bid_length, false);
    ptr->sequence_no = quote.sequence_no;

    // 检查发布频率
    //if( symbol == "BTC_USDT" ) {
    //    std::cout << quote.exchange << " " << quote.ask_length << " " << quote.asks[0].price.get_str_value() << " " << 
    //    ptr->asks->price.get_str_value() << " " << ptr->asks->next->price.get_str_value() << " " << ptr->asks->next->next->price.get_str_value() << std::endl;
    //}
    if( !_check_update_clocks(symbol) ) {
        return false;
    }

    std::cout << "update " << symbol << " " << ptr->ask_length() << "/" << ptr->bid_length() << std::endl;
    pub_snap = mixquote_to_pbquote2("", symbol, ptr, true);
    return true;
}

bool QuoteMixer2::_get_quote(const TSymbol& symbol, SMixQuote*& ptr) const {
    auto iter = quotes_.find(symbol);
    if( iter == quotes_.end() )
        return false;
    ptr = iter->second;
    return true;
}

SMixDepthPrice* QuoteMixer2::_clear_pricelevel(const TExchange& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
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

SMixDepthPrice* QuoteMixer2::_clear_exchange(const TExchange& exchange, SMixDepthPrice* depths) {
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

SMixDepthPrice* QuoteMixer2::_mix_exchange(const TExchange& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
    const int& length, bool isAsk) { 
    SMixDepthPrice head;
    head.next = mixedDepths;
    SMixDepthPrice* last = &head;
    SMixDepthPrice* tmp = mixedDepths;

    // 1. 混合
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

bool QuoteMixer2::_preprocess(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    process_precise(exchange, symbol, src, dst);
    return true;
}
