#pragma once

#include "redis_kline_source.h"

class IMixerKlinePusher
{
    virtual void on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& kline) = 0;
};

class IDataProvider
{
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline) = 0;
};

class KlineMixer : public IRedisKlineSource 
{
public:
    KlineMixer();
    ~KlineMixer();

    void start();

    void register_callback(IMixerKlinePusher* callback) { callbacks_.insert(callback); }

    void set_db_interface(IDataProvider* db_interfacce) { db_interface_ = db_interfacce; }

    // IRedisKlineSource
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline);

private:
    set<IMixerKlinePusher*> callbacks_;
    IDataProvider* db_interface_;
};
