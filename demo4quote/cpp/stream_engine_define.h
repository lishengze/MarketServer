#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <chrono>
using namespace std;
#include "redis_quote_define.h"


// 内部行情结构
struct SMixDepthPrice {
    SDecimal price;
    unordered_map<TExchange, SDecimal> volume;
    SMixDepthPrice* next;

    SMixDepthPrice() {
        next = NULL;
    }
};

struct SMixQuote {
    SMixDepthPrice* asks; // 卖盘
    SMixDepthPrice* bids; // 买盘
    SDecimal watermark;
    long long sequence_no;

    SMixQuote() {
        asks = NULL;
        bids = NULL;
    }

    unsigned int ask_length() const {
        return _get_length(asks);
    }

    unsigned int bid_length() const {
        return _get_length(bids);
    }

    unsigned int _get_length(const SMixDepthPrice* ptr) const {
        unsigned int ret = 0;
        while( ptr != NULL ) {
            ptr = ptr->next;
            ret ++;
        }
        return ret;
    }

    void release() {
        _release(asks);
        _release(bids);
        asks = NULL;
        bids = NULL;
    }

    void _release(SMixDepthPrice* ptr) {            
        SMixDepthPrice *tmp = ptr;
        while( tmp != NULL ) {
            // 删除             
            SMixDepthPrice* waitToDel = tmp;
            tmp = tmp->next;
            delete waitToDel;
        }
    }
};

struct SNacosConfigByExchange
{
    int fee_type;
    float fee_maker;
    float fee_taker;
    int precise;    // 最小价格单位
    int vprecise;   // 最小挂单量单位
};

struct SNacosConfig
{
    type_uint32 depth; // 【原始】深度
    float frequency;    // 【原始】更新频率
    int precise;    // 【聚合】最小价格单位
    int vprecise;   // 【聚合】最小挂单量单位
    type_uint32 mix_depth;  // 【聚合】深度
    float mix_frequecy; // 【聚合】更新频率
    unordered_map<TExchange, SNacosConfigByExchange> exchanges;    

    unordered_set<TExchange> get_exchanges() const {
        unordered_set<TExchange> ret;
        for( const auto& v : exchanges ) {
            ret.insert(v.first);
        }
        return ret;
    }
};

struct SymbolFee
{
    int fee_type;       // 0表示fee不需要处理（默认），1表示fee值为比例，2表示fee值为绝对值
    double maker_fee;
    double taker_fee;

    SymbolFee() {
        fee_type = 0;
        maker_fee = taker_fee = 0.2;
    }

    void compute(const SDecimal& src, SDecimal& dst, bool is_ask) const
    {
        if( fee_type == 2 ) {
            if( is_ask ) {
                dst = src.add(maker_fee, true);
            } else {
                dst = src.add(taker_fee, false);
            }
        } else if( fee_type == 1 ) {
            if( is_ask ) {
                dst = src.multiple((100 + maker_fee) / 100.0, true);
            } else {
                dst = src.multiple((100 - taker_fee) / 100.0, false);
            }
        } else {
            dst = src;
        }
    }
};
