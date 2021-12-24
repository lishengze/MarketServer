#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"
#include "struct_define.h"
#include "risk_interface_define.h"

class DepthRiskCtrl:public bcts::comm::QuoteSourceCallbackInterface, 
                    public IConfigurationUpdater,
                    public IOrderUpdater,
                    public IAccountUpdater
{
public:

    DepthRiskCtrl();
    ~DepthRiskCtrl();

    void start();

    void set_comm(bcts::comm::Comm*  comm){ p_comm_ = comm;}

    virtual void on_snap( SDepthQuote& quote);

    virtual void on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config);

    virtual void on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config);

    virtual void on_hedge_config_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config);

    // 账户相关回调
    virtual void on_account_update(const AccountInfo& info);

    // 未对冲订单簿更新
    virtual void on_order_update(const string& symbol, 
                            const SOrder& order, 
                            const vector<SOrderPriceLevel>& asks, 
                            const vector<SOrderPriceLevel>& bids);

private:


private:

    bcts::comm::Comm*                                               p_comm_{nullptr};

    // 计算线程
    std::thread*                                                    thread_loop_{nullptr};
    std::atomic<bool>                                               thread_run_;

    // 配置信息
    mutable std::mutex                                              mutex_config_;
    unordered_map<TSymbol, SMixerConfig>                            configs_; 

    // 控制频率    
    unordered_map<TSymbol, type_tick>                               last_clocks_;

    mutable std::mutex                                              mutex_quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>>   quotes_;
};
