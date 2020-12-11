#pragma once

#include "stream_engine_define.h"

class IDataProvider
{
public:
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines) = 0;
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init) = 0;
};

class KlineDatabase : public IDataProvider
{
public:
    KlineDatabase();
    ~KlineDatabase();

    void start();

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init);
private:

    mutable std::mutex mutex_caches_;
    unordered_map<TSymbol, unordered_map<int, vector<KlineData>>> caches_;
};
