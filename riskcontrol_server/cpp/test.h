#pragma once

#include "comm.h"
#include "depth_riskctrl.h"
#include "updater_account.h"
#include "updater_order.h"

#include "nacos_config.h"
#include "risk_interface_define.h"

class TestEngine:public IConfigurationUpdater
{
    public:
        TestEngine(string server_address):server_address_{server_address}
        {
            init();
        }

        ~TestEngine();

        void init();

        void start();

        bool is_symbol_updated(const map<TSymbol, SymbolConfiguration>& config);

        virtual void on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config);

        virtual void on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config);

        virtual void on_hedge_config_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config);

    private:
        string                                           server_address_{nullptr};

        bcts::comm::Comm*                                p_comm_server_{nullptr};
        DepthRiskCtrl*                                   p_depth_riskctrl_{nullptr};

        NacosConfig                                      nacos_config_;

        AccountUpdater                                   account_updater_;

        OrderUpdater                                     order_updater_;

        std::map<TSymbol, std::set<TExchange>>           meta_map_;
};


void TestMain();