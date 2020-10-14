#pragma once

#include "stream_engine_define.h"

class QuoteSingle
{
public:
    QuoteSingle(){}

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void publish_quote(const string& exchange, const string& symbol, const SMixQuote& quote, bool isSnap);

private:
    bool _get_quote(const string& exchange, const string& symbol, SMixQuote*& ptr) const {
        auto iter = symbols_.find(exchange);
        if( iter == symbols_.end() )
            return false;
        auto iter2 = iter->second.find(symbol);
        if( iter2 == iter->second.end() )
            return false;        
        ptr = iter2->second;
        return true;
    }

    unordered_map<TExchange, unordered_map<TSymbol, SMixQuote*>> symbols_;
    unordered_map<TExchange, unordered_map<TSymbol, long long>> last_clocks_;
    
    SMixDepthPrice* _clear_allpricelevel(const string& exchange, SMixDepthPrice* depths) {
        SMixDepthPrice *tmp = depths;
        while( tmp != NULL ) {
            // 删除             
            SMixDepthPrice* waitToDel = tmp;
            tmp = tmp->Next;
            delete waitToDel;
        }
        return NULL;
    }

    
    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
        SMixDepthPrice head;
        head.Next = depths;        
        SMixDepthPrice *tmp = depths, *last = &head;

        for( int i = 0 ; i < newLength ; ++i ) {
            const SDepthPrice& depth = newDepths[i];
            if( depth.Volume > VOLUME_PRECISE ) {
                continue;
            }
            // 找到并删除对应价位
            while( tmp != NULL ) {
                if( tmp->Price == depth.Price ) { 
                    // 删除             
                    SMixDepthPrice* waitToDel = tmp;
                    last->Next = tmp->Next;
                    tmp = tmp->Next;
                    delete waitToDel;
                    break;
                } else if ( isAsk ? (tmp->Price > depth.Price) : (tmp->Price < depth.Price) ) {
                    break;
                }
                last = tmp;
                tmp = tmp->Next;
            }
        }
        return head.Next;
    }

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk) { 
        SMixDepthPrice head;
        head.Next = mixedDepths;
        SMixDepthPrice* last = &head;

        // 4. 混合：first + depths
        int i = 0;
        SMixDepthPrice* tmp = mixedDepths;
        for( tmp = head.Next ; i < length && tmp != NULL ; ) {
            const SDepthPrice& depth = depths[i];
            if( depth.Volume < VOLUME_PRECISE ) {
                ++i;
                continue;
            }
            if( isAsk ? (depth.Price < tmp->Price) : (depth.Price > tmp->Price) ) {
                // 新建价位
                SMixDepthPrice *newDepth = new SMixDepthPrice();
                newDepth->Price = depth.Price;
                newDepth->Volume[exchange] = depth.Volume;
                
                last->Next = newDepth;
                last = newDepth;
                newDepth->Next = tmp;
                ++i;
            } else if( depth.Price == tmp->Price ) {
                tmp->Volume[exchange] = depth.Volume;
                ++i;
            } else {
                last = tmp;
                tmp = tmp->Next;
            }
        }

        // 5. 剩余全部加入队尾
        for( ; i < length ; ++i) {
            const SDepthPrice& depth = depths[i];
            if( depth.Volume < VOLUME_PRECISE ) {
                continue;
            }
            // 新建价位
            SMixDepthPrice *newDepth = new SMixDepthPrice();
            newDepth->Price = depth.Price;
            newDepth->Volume[exchange] = depth.Volume;
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
