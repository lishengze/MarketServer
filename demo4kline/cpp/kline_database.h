#pragma once

#include "kline_mixer.h"

class KlineDatabase : public IMixerKlinePusher, public IDataProvider
{
public:
    KlineDatabase();
    ~KlineDatabase();

    void start();

    // IMixerKlinePusher
    void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& kline);

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);
private:

};
