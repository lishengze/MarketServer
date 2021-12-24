#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "data_struct/data_struct.h"

class IConfigurationUpdater {
public:
    virtual void on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config) = 0;

    virtual void on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config) = 0;

    virtual void on_hedge_config_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config) = 0;
};

class IOrderUpdater {
public:
    virtual void on_order_update(const TSymbol& symbol, const SOrder& order, 
    const vector<SOrderPriceLevel>& asks, 
    const vector<SOrderPriceLevel>& bids) = 0;
};

class IAccountUpdater {
public:
    virtual void on_account_update(const AccountInfo& info) = 0;
};