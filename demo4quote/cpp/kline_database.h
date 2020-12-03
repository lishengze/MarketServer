#pragma once

#include "kline_mixer.h"

class KlineDatabase : public IMixerKlinePusher, public IDataProvider
{
public:
    KlineDatabase();
    ~KlineDatabase();

    void start();

    // IMixerKlinePusher
    void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines);

    // IDataProvider
    bool get_kline(const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);
private:

    mutable std::mutex mutex_caches_;
    unordered_map<TSymbol, unordered_map<int, vector<KlineData>>> caches_;
};
