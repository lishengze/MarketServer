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

    void set_meta(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    void set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& meta_map);

    bool add_kline(const KlineData& input, KlineData& output);

    void set_config(unordered_map<TSymbol, SMixerConfig>& symbol_config)
    {
        symbol_config_ = symbol_config;
    }

private:
    mutable std::mutex                                              mutex_cache_;
    unordered_map<TSymbol, unordered_map<TExchange, CalcCache*>>    caches_;
    unordered_map<TSymbol, SMixerConfig>                            symbol_config_;
};



class KlineProcessor
{
public:

    KlineProcessor(QuoteSourceCallbackInterface * engine):engine_{engine} {}

    ~KlineProcessor();

    void process(KlineData& src);

    void set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& meta_map);

    void config_process(KlineData& src);

    void set_config(unordered_map<TSymbol, SMixerConfig>& symbol_config);

private:
    KlineAggregater                     min1_kline_aggregator_;

    KlineAggregater                     min60_kline_aggregator_;

    QuoteSourceCallbackInterface *      engine_{nullptr};
};