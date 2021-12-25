#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"
#include "risk_interface_define.h"
#include "native_config.h"
#include "riskcontrol_worker.h"

#include "util/tool.h"
#include "global_declare.h"

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

public:
    bool get_snaps(vector<SDepthQuote>& snaps);

    void get_params(map<TSymbol, SDecimal>& watermarks, 
                    map<TExchange, map<TSymbol, double>>& accounts, 
                    map<TSymbol, string>& configurations);

    bool check_quote(SDepthQuote& quote);

    virtual void hedge_trade_order(string& symbol, double price, double amount, 
                                    TradedOrderStreamData_Direction direction, 
                                    bool is_trade);

private:

    void _publish_quote(const SDepthQuote& quote);

    void _push_to_clients(const string& symbol = "");

    mutable std::mutex                      mutex_datas_;
    unordered_map<TSymbol, SDepthQuote>     datas_;
    unordered_map<TSymbol, SDepthQuote>     last_datas_;
    Params                                  params_;

    // 处理流水线
    QuotePipeline                           pipeline_;

    AccountAjdustWorker                     account_worker_;
    OrderBookWorker                         orderbook_worker_;
    QuoteBiasWorker                         quotebias_worker_;
    WatermarkComputerWorker                 watermark_worker_;
    PrecisionWorker                         pricesion_worker_;

    std::mutex                              filter_quote_mutex_;
private:
    bcts::comm::Comm*                       p_comm_{nullptr};
};
