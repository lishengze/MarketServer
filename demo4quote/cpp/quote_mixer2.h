#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"

/*
算法：源头的行情更新全部更新到品种的买卖盘队列中，由独立线程计算买卖盘分水岭位置，保持分水岭相对稳定可用，在发布聚合行情的时候，再基于分水岭位置进行行情价位过滤
源头更新回调：全量更新、增量更新都需要加锁
计算线程：快照数据全量加锁
*/
class QuoteMixer2
{
public:
    QuoteMixer2() {
        thread_loop_ = new std::thread(&QuoteMixer2::_calc_watermark, this);
    }   

    ~QuoteMixer2() {
        if (thread_loop_) {
            if (thread_loop_->joinable()) {
                thread_loop_->join();
            }
            delete thread_loop_;
        }
    }
    
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

private:
    // 独立线程计算watermark
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, SDecimal> watermark_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> snaps_;
    std::thread*               thread_loop_ = nullptr;
    void _calc_watermark();
    void _one_round();
    bool _get_watermark(const string& symbol, SDecimal& watermark) const;

    // symbols
    unordered_map<TSymbol, SMixQuote*> symbols_;
    unordered_map<TSymbol, long long> last_clocks_;


    bool _get_quote(const string& symbol, SMixQuote*& ptr) const {
        auto iter = symbols_.find(symbol);
        if( iter == symbols_.end() )
            return false;
        ptr = iter->second;
        return true;
    }

    std::shared_ptr<QuoteData> _publish_quote(const string& symbol, const SMixQuote* quote, bool isSnap);

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
                            break;
                        }
                    }
                } else if ( isAsk ? (tmp->Price > depth.Price) : (tmp->Price < depth.Price) ) {
                    break;
                }
                last = tmp;
                tmp = tmp->Next;
            }
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

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, const SDecimal& watermark, bool isAsk) { 
        SMixDepthPrice head;
        head.Next = mixedDepths;
        SMixDepthPrice* last = &head;
        SMixDepthPrice* tmp = mixedDepths;

        // 4. 混合：first + depths
        int i = 0;
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