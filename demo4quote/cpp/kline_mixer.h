#pragma once

#include "redis_quote_define.h"

class IMixerKlinePusher
{
public:
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines) = 0;
};

class IDataProvider
{
public:
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines) = 0;
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init) = 0;
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

#define KLINE_CACHE_MIN1 1440
#define KLINE_CACHE_MIN60 240

class KlineCache
{
public:
    KlineCache();
    ~KlineCache();

    void set_limit(size_t limit) { limit_ = limit; }

    void update_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& klines);
private:
    size_t limit_;

    void _shorten(vector<KlineData>& datas);

    mutable std::mutex mutex_data_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> data_;
};

class KlineMixer
{
public:
    KlineMixer();
    ~KlineMixer();

    void set_engine(QuoteSourceInterface* ptr) { engine_interface_ = ptr; }

    void set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);
private:
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;
    
    MixCalculator min1_kline_calculator_;
    MixCalculator min60_kline_calculator_;

    unordered_map<TSymbol, unordered_map<TExchange, bool>> kline1min_firsttime_;
    unordered_map<TSymbol, unordered_map<TExchange, bool>> kline60min_firsttime_;
};

class KlineHubber
{
public:
    KlineHubber();
    ~KlineHubber();

    void register_callback(IMixerKlinePusher* callback) { callbacks_.insert(callback); }

    void set_db_interface(IDataProvider* db_interfacce) { db_interface_ = db_interfacce; }

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init);

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);

private:
    set<IMixerKlinePusher*> callbacks_;
    IDataProvider* db_interface_;

    KlineCache min1_cache_;
    KlineCache min60_cache_;
};
