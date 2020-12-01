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


// 内部行情结构（链表）
struct SMixDepthPrice {
    SDecimal price;
    unordered_map<TExchange, double> volume;
    SMixDepthPrice* next;

    SMixDepthPrice() {
        next = NULL;
    }
};

#define MAX_MIXDEPTH 999999
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