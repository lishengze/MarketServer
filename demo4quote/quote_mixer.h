#pragma once

#include "stream_engine_define.h"
#include "grpc_publisher.h"

class QuoteMixer
{
public:
public:
    QuoteMixer(){
        publisher_.init();
    }

    void on_mix_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
        if( symbol == "BTC_USDC" ){
            cout << "on_mix_snap exchange-" << exchange << " time-" << quote.TimeArrive << endl;
        } else {
         //   return;
        }
        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) ) {
            ptr = new SMixQuote();
            symbols_[symbol] = ptr;
            cout << "create symbol " << symbol << " new " << ptr << endl;
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
        publish_quote(symbol, *ptr);
        return;
    }

    void on_mix_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
        if( symbol == "BTC_USDC" ){
            cout << "on_mix_update exchange-" << exchange << " time-" << quote.TimeArrive << endl;
        }
        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) )
            return;
        SDepthQuote newQuote = quote;
        // 1. 需要清除的价位数据
        ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, newQuote.Asks, newQuote.AskLength);
        ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, newQuote.Bids, newQuote.BidLength);
        // 2. 修剪cross的价位
        _cross_askbid(ptr, newQuote);
        // 3. 合并价位
        ptr->Asks = _mix_exchange(exchange, ptr->Asks, newQuote.Asks, newQuote.AskLength, ptr->Watermark, true);
        ptr->Bids = _mix_exchange(exchange, ptr->Bids, newQuote.Bids, newQuote.BidLength, ptr->Watermark, false);
        // 4. 推送结果
        publish_quote(symbol, *ptr);
        return;
    }

    void publish_quote(const string& symbol, const SMixQuote& quote) {
        if( symbol == "BTC_USDC" ){
            cout << "publish_quote symbol-" << symbol << endl;
        }
        publisher_.publish(symbol, quote);
    }

private:

    // trade client
    GrpcPublisher publisher_;

    // symbols
    unordered_map<TSymbol, SMixQuote*> symbols_;

    bool _get_quote(const string& symbol, SMixQuote*& ptr) const {
        auto iter = symbols_.find(symbol);
        if( iter == symbols_.end() )
            return false;
        ptr = iter->second;
        return true;
    }

    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, int& newLength) {
        if( newLength == 0 )
            return depths;
        const SDepthPrice& level = newDepths[0];
        if( level.Volume > 0.00001 ) {
            return depths;
        }
        newLength = 0;

        // 删除对应价位的挂单
        SMixDepthPrice head;
        head.Next = depths;        
        SMixDepthPrice *tmp = depths, *last = &head;
        while( tmp != NULL ) {
            if( tmp->Price == level.Price ) {                
                // 找到对应的价位
                unordered_map<TExchange, double>& volumeByExchange = tmp->Volume;
                auto iter = volumeByExchange.find(exchange);
                if( iter != volumeByExchange.end() ) {
                    volumeByExchange.erase(iter);                
                    if( volumeByExchange.size() == 0 ) {
                        // 删除             
                        SMixDepthPrice* waitToDel = tmp;
                        last->Next = tmp->Next;
                        tmp = tmp->Next;
                        delete waitToDel;
                        continue;
                    }
                }
                break;
            }
            last = tmp;
            tmp = tmp->Next;
        }
        return head.Next;
    }

    SMixDepthPrice* _clear_exchange(const string& exchange, SMixDepthPrice* depths) {
        SMixDepthPrice head;
        head.Next = depths;
        
        SMixDepthPrice *tmp = depths, *last = &head;
        while( tmp != NULL ) {
            unordered_map<TExchange, double>& volumeByExchange = tmp->Volume;
            auto iter = volumeByExchange.find(exchange);
            if( iter != volumeByExchange.end() ) {
                volumeByExchange.erase(iter);                
                if( volumeByExchange.size() == 0 ) {
                    // 删除             
                    SMixDepthPrice* waitToDel = tmp;
                    last->Next = tmp->Next;
                    tmp = tmp->Next;
                    delete waitToDel;
                    continue;
                }
            }
            last = tmp;
            tmp = tmp->Next;
        }
        return head.Next;
    }

    void _cross_askbid(SMixQuote* mixedQuote, const SDepthQuote& quote) {
        // 新行情的买1价 大于 聚合行情的卖1价
        if( mixedQuote->Asks != NULL && quote.BidLength > 0 && mixedQuote->Asks->Price < quote.Bids[0].Price ) {
            mixedQuote->Watermark = (quote.Bids[0].Price - mixedQuote->Asks->Price) / 2;
        }
        // 新行情的卖1价 小于 聚合行情的买1价
        else if( mixedQuote->Bids != NULL && quote.AskLength > 0 && mixedQuote->Bids->Price > quote.Asks[0].Price ) {
            mixedQuote->Watermark = (mixedQuote->Bids->Price - quote.Asks[0].Price) / 2;
        }
        else {
            mixedQuote->Watermark = SDecimal();
        }
    }

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, const SDecimal& watermark, bool isAsk) { 
        SMixDepthPrice head;
        head.Next = mixedDepths;
        SMixDepthPrice* last = &head;

        // 1. 裁剪聚合行情对应价位（开区间）
        SMixDepthPrice* tmp = mixedDepths;
        while( tmp != NULL ) {
            if( watermark.Value == 0 || (isAsk ? (tmp->Price > watermark) : (tmp->Price < watermark)) ) {
                break;
            } else {
                SMixDepthPrice* waitToDel = tmp;
                // 删除
                last->Next = tmp->Next;       
                tmp = tmp->Next;
                delete waitToDel;
            }
        }

        // 2. 裁剪新行情对应价位（闭区间）
        int filteredLevel = 0;
        double virtualVolume; // 虚拟价位挂单量
        for( int i = 0 ; i < length ; ++ i ) {
            if( watermark.Value == 0 || (isAsk ? (depths[i].Price >= watermark) : (depths[i].Price <= watermark)) ) {
                break;        
            } else {
                filteredLevel = i;
                virtualVolume += depths[i].Volume;
            }
        }

        // 3. 插入虚拟价位
        if( virtualVolume > 0 && watermark.Value > 0 ) {
            SMixDepthPrice* virtualDepth = new SMixDepthPrice();
            virtualDepth->Price = watermark;
            virtualDepth->Volume[exchange] = virtualVolume;
            // 加入链表头部
            virtualDepth->Next = head.Next;
            head.Next = virtualDepth;
        }

        // 4. 混合：first + depths
        int i = filteredLevel;
        for( tmp = head.Next ; i < length && tmp != NULL ; ) {
            const SDepthPrice& depth = depths[i];
            if( isAsk ? (depth.Price < tmp->Price) : (depth.Price > tmp->Price) ) {
                // 新建价位
                SMixDepthPrice *newDepth = new SMixDepthPrice();
                newDepth->Price = depth.Price;
                newDepth->Volume[exchange] += depth.Volume;
                
                last->Next = newDepth;
                last = newDepth;
                newDepth->Next = tmp;
                ++i;
            } else if( depth.Price == tmp->Price ) {
                tmp->Volume[exchange] += depth.Volume;
                ++i;
            } else {
                last = tmp;
                tmp = tmp->Next;
            }
        }

        // 5. 剩余全部加入队尾
        for( ; i < length ; ++i) {
            const SDepthPrice& depth = depths[i];
            // 新建价位
            SMixDepthPrice *newDepth = new SMixDepthPrice();
            newDepth->Price = depth.Price;
            newDepth->Volume[exchange] += depth.Volume;
            newDepth->Next = NULL;
            // 插入
            last->Next = newDepth;
            last = newDepth;
        }

        // 6. 删除多余的价位
        tmp = head.Next;
        int count = 0;
        while( tmp != NULL && count < MAX_MIXDEPTH) {
            tmp = tmp->Next;
            count ++;
        }
        if( tmp != NULL ) {
            SMixDepthPrice* deletePtr = tmp->Next;
            tmp->Next = NULL;
            while( deletePtr != NULL ) {
                SMixDepthPrice *waitToDelete = deletePtr;
                deletePtr = deletePtr->Next;
                delete waitToDelete;
            }
        }

        return head.Next;
    }
};