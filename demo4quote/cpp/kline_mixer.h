#pragma once

#include "struct_define.h"
#include "kline_database.h"

// K线往下游推送接口
class IKlinePusher
{
public:
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines) = 0;
};

/*
缓存接口
*/
class IKlineCacher
{
public:
    // 请求缓存中的K线
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines) = 0;
    // （首次连接）提取已在缓存中的数据
    virtual void fill_cache(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min1, unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min60) = 0;
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

#define KLINE_CACHE_MIN1 2880
#define KLINE_CACHE_MIN60 240
class KlineCache
{
public:
    KlineCache();
    ~KlineCache();

    void set_limit(size_t limit) { limit_ = limit; }
    void set_resolution(uint32 resolution) { resolution_ = resolution; }

    void update_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& klines, vector<KlineData>& outputs, vector<KlineData>& output_60mins);

    void fill_klines(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache);
private:
    size_t limit_;
    uint32 resolution_;

    void _shorten(vector<KlineData>& datas);

    mutable std::mutex mutex_data_;

public:    
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> data_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> data_60min_;
};

class KlineHubber : public IKlineCacher
{
public:
    KlineHubber();
    ~KlineHubber();

    void register_callback(IKlinePusher* callback) { callbacks_.insert(callback); }

    void set_db_interface(IDataProvider* db_interfacce) { db_interface_ = db_interfacce; }

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init, vector<KlineData>& outputs, bool is_restart=false);

    // IDataCacher
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);
    void fill_cache(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min1, unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min60);

    // restart from database;
    void recover_from_db();

    void start();


private:
    set<IKlinePusher*> callbacks_;
    IDataProvider* db_interface_;

    KlineCache min1_cache_;
    KlineCache min60_cache_;

    std::thread   recover_thread_;
};

// 计算聚合K线
class KlineMixer
{
public:
    KlineMixer();
    ~KlineMixer();

    void set_engine(QuoteSourceCallbackInterface* ptr) { engine_interface_ = ptr; }

    void set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges);

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init);
private:
    // callback
    QuoteSourceCallbackInterface *engine_interface_ = nullptr;
    
    MixCalculator min1_kline_calculator_;
    MixCalculator min60_kline_calculator_;

    unordered_map<TSymbol, unordered_map<TExchange, bool>> kline1min_firsttime_;
    unordered_map<TSymbol, unordered_map<TExchange, bool>> kline60min_firsttime_;
};
