#pragma once

#include "comm.h"
#include "depth_aggregater.h"
#include "kline_aggregater.h"
#include "trade_aggregater.h"

#include "configuration_client.h"

class TestEngine:public INacosCallback
{
    public:
        TestEngine(string server_address):server_address_{server_address}
        {
            init();
        }

        ~TestEngine();

        void init();

        void start();

        bool parse_config(const Document& symbols, 
                            std::map<TSymbol, SNacosConfig>& curr_config);

        void update_config(const std::map<TSymbol, SNacosConfig>& curr_config,
                           bool& is_config_changed, bool& is_new_sub_info);

        void notify_config_change();

        void notify_meta_change();

        virtual void on_config_channged(const Document& symbols);

    private:
        string                                           server_address_{nullptr};

        bcts::comm::Comm*                                p_comm_server_{nullptr};
        DepthAggregater*                                 p_depth_aggregater_{nullptr};
        KlineAggregater*                                 p_kline_aggregater_{nullptr};
        TradeAggregater*                                 p_trade_aggregater_{nullptr};

        ConfigurationClient                              config_client_;

        std::map<TSymbol, SNacosConfig>                  nacos_config_;
        std::unordered_map<TSymbol, SMixerConfig>        mixer_config_;

        std::map<TSymbol, std::set<TExchange>>           meta_map_;
};


void TestMain();