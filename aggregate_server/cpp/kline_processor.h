#pragma once

#include "global_declare.h"

#include "struct_define.h"

#include "interface_define.h"

#define INVALID_INDEX ((type_tick)(-1))

class KlineAggregater
{
public:
    struct CalcCache{
        list<KlineData> klines;

        type_tick get_last_index() const {
            return klines.size() > 0 ? klines.back().index : INVALID_INDEX;
        }
        type_tick get_first_index() const {
            return klines.size() > 0 ? klines.front().index : INVALID_INDEX;
        }
        void clear_earlier_kline(type_tick index) {
            while( klines.size() > 0 ) {
                if( klines.front().index < index ) {
                    klines.pop_front();
                } else {
                    break;
                }
            }
        }

        KlineData get_index(type_tick index) {
            while( klines.size() > 0 ) {
                if( klines.front().index < index ) {
                    klines.pop_front();
                } else if( klines.front().index == index ) {
                    return klines.front();
                } else {
                    break;
                }
            }
            return KlineData();
        }
    };

    KlineAggregater();
    ~KlineAggregater();

    void set_config(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    bool add_kline(const KlineData& input, KlineData& output);

private:
    mutable std::mutex                                              mutex_cache_;
    unordered_map<TSymbol, unordered_map<TExchange, CalcCache*>>    caches_;
};



class KlineProcessor
{
public:

    KlineProcessor();

    ~KlineProcessor();

    void process(KlineData& src);

    void set_config(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

private:
    KlineAggregater min1_kline_aggregator_;

    KlineAggregater min60_kline_aggregator_;

    QuoteSourceCallbackInterface *                                  engine_{nullptr};
};