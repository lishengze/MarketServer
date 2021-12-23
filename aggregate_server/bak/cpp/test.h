#pragma once
#include "kafka_server.h"
#include "decode_processer.h"
#include "depth_processor.h"
#include "kline_processor.h"
#include "trade_processor.h"
#include "depth_aggregater.h"

#include "pandora/util/io_service_pool.h"
#include "configuration_client.h"
#include "interface_define.h"

class TestEngine:public INacosCallback
{
    public:
        TestEngine(utrade::pandora::io_service_pool& engine_pool):
        engine_pool_{engine_pool}
        {
            init();
        }

        ~TestEngine();

        void init();

        void start();

        bool parse_config(const Document& symbols, 
                            std::unordered_map<TSymbol, SNacosConfig>& curr_config);

        void update_config(const std::unordered_map<TSymbol, SNacosConfig>& curr_config,
                           bool& is_config_changed, bool& is_new_sub_info);

        void notify_config_change();

        void notify_meta_change();

        virtual void on_config_channged(const Document& symbols);

    private:
        KafkaServer*                                    p_kafka_{nullptr};
        DecodeProcesser*                                p_decode_processer_{nullptr}; 
        EncodeProcesser*                                p_encode_processer_{nullptr};

        DepthProcessor*                                 p_depth_processor_{nullptr};
        KlineProcessor*                                 p_kline_processor_{nullptr};
        TradeProcessor*                                 p_trade_processor_{nullptr};

        utrade::pandora::io_service_pool&               engine_pool_;
        ConfigurationClient                             config_client_;

        std::unordered_map<TSymbol, SNacosConfig>       nacos_config_;
        std::unordered_map<TSymbol, SMixerConfig>       mixer_config_;

        std::unordered_map<TSymbol, std::set<TExchange>>meta_map_;
};


void TestMain();