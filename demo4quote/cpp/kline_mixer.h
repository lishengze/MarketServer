#pragma once

#include "redis_quote_define.h"

class IMixerKlinePusher
{
public:
    virtual void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines) = 0;
};

class IDataProvider
{
public:
    virtual bool get_kline(const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines) = 0;
};


#define INVALID_INDEX ((type_tick)(-1))
class MixCalculator
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
        void clear(type_tick index) {
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
            // never reach here
            cout << "fatal error" << endl;
            return KlineData();
        }
    };



    MixCalculator();
    ~MixCalculator();

    void set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);
    bool add_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& input, vector<KlineData>& output);
private:
    mutable std::mutex mutex_cache_;
    unordered_map<TSymbol, unordered_map<TExchange, CalcCache*>> caches_;
};

class KlineMixer
{
public:
    KlineMixer();
    ~KlineMixer();

    void start();

    void set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    void register_callback(IMixerKlinePusher* callback) { callbacks_.insert(callback); }

    void set_db_interface(IDataProvider* db_interfacce) { db_interface_ = db_interfacce; }

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);

    // IDataProvider
    bool get_kline(const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);

private:
    set<IMixerKlinePusher*> callbacks_;
    IDataProvider* db_interface_;

    MixCalculator min1_kline_calculator_;
    MixCalculator min60_kline_calculator_;

};
