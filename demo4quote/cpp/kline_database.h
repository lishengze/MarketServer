#pragma once

#include "kline_mixer.h"

class KlineDatabase : public IDataProvider
{
public:
    KlineDatabase();
    ~KlineDatabase();

    void start();

    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init);

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);
private:

    mutable std::mutex mutex_caches_;
    unordered_map<TSymbol, unordered_map<int, vector<KlineData>>> caches_;
};
