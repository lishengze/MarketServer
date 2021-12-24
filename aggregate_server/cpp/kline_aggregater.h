#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"

#include "struct_define.h"

#define INVALID_INDEX ((type_tick)(-1))

class KlineAggregater:public bcts::comm::QuoteSourceCallbackInterface
{
public:
    void set_comm(bcts::comm::Comm*  comm){ p_comm_ = comm;}

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
    virtual ~KlineAggregater();

    void set_meta(const std::map<TSymbol, std::set<TExchange>>& meta_map);

    void update_cache_meta(const std::map<TSymbol, std::set<TExchange>>& added_meta,
                           const std::map<TSymbol, std::set<TExchange>>& removed_meta);

    void get_delata_meta(const std::map<TSymbol, std::set<TExchange>>& new_meta_map,
                        std::map<TSymbol, std::set<TExchange>>& added_meta,
                        std::map<TSymbol, std::set<TExchange>>& removed_meta);


    bool is_exchange_added_to_aggregate(const KlineData& exchange, const KlineData& input);

    bool add_kline(const KlineData& input, KlineData& output);

    void set_config(unordered_map<TSymbol, SMixerConfig>& symbol_config)
    {
        symbol_config_ = symbol_config;
    }

    virtual void on_kline( KlineData& kline);
private:
    bcts::comm::Comm*                                               p_comm_{nullptr};

    mutable std::mutex                                              mutex_cache_;
    map<TSymbol, map<TExchange, CalcCache*>>                        caches_;
    unordered_map<TSymbol, SMixerConfig>                                      symbol_config_;
};
