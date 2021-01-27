#pragma once

#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"
using namespace nacos;

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
using namespace std;
#include "pandora/util/json.hpp"
using njson = nlohmann::json;
#include "risk_controller_define.h"
#include "risk_controller_config.h"

struct QuoteConfiguration
{
    //double MakerFee; // 百分比；maker手续费
    //double TakerFee; // 百分比；taker手续费
    double PriceBias; // 百分比；价格偏移量（买卖盘相同）
    double VolumeBias; // 百分比；单量偏移量（买卖盘相同）
    //double UserPercent; // 百分比；用户账户可动用比例
    double HedgePercent; // 百分比；对冲账户可动用比例
    double OtcBias; // 百分比，otc查询

    string desc() const {
        return tfm::format("priceBias=%.2f volumeBias=%.2f otcBias=%.2f hedge=%.2f", PriceBias, VolumeBias, OtcBias, HedgePercent);
    }
};

class IConfigurationUpdater {
public:
    virtual void on_configuration_update(const map<TSymbol, QuoteConfiguration>& config) = 0;
};

class NacosListener : public Listener 
{
private:
    string group_;
    string dataid_;
    ConfigService* server_;
    IConfigurationUpdater* callback_;
public:
    NacosListener(ConfigService* server, string group, string dataid, IConfigurationUpdater* callback) {    
        group_ = group;
        dataid_ = dataid;
        server_ = server;
        callback_ = callback;

        NacosString ss = "";
        try {
            ss = server_->getConfig(dataid_, group_, 1000);
        }
        catch (NacosException &e) {
            _log_and_print("[fatal] Request failed with curl code:%d Reason:%s", e.errorcode(), e.what());
            return;
        }
        on_get_config(ss);
    }

    void on_get_config(const NacosString &configInfo) const {        
        if( group_ == "riskcontrol" && dataid_ == "symbols" ) {
            _log_and_print("on_get_config: %s", configInfo);
            // json 解析
            njson js;    
            try
            {
                js = njson::parse(configInfo);
            }
            catch(nlohmann::detail::exception& e)
            {
                _log_and_print("[fatal] parse json fail %s", e.what());
                return;
            }    
            _log_and_print("parse config from nacos finish");

            map<TSymbol, QuoteConfiguration> config;            
            for (auto iter = js.begin() ; iter != js.end() ; ++iter )
            {
                const TSymbol& symbol = iter.key();
                const njson& symbol_cfgs = iter.value();
                int enable = symbol_cfgs["enable"].get<int>();
                if( enable < 1 )
                    continue;
                QuoteConfiguration cfg;
                cfg.PriceBias = symbol_cfgs["price_bias"].get<double>();
                cfg.VolumeBias = symbol_cfgs["volume_bias"].get<double>();
                cfg.HedgePercent = symbol_cfgs["hedge_percent"].get<double>();
                cfg.OtcBias = symbol_cfgs["otc_bias"].get<double>();
                config[symbol] = cfg;
            }
            callback_->on_configuration_update(config);
        }
    }

    void receiveConfigInfo(const NacosString &configInfo) {
        on_get_config(configInfo);
    }
};

class ConfigurationUpdater {
public:
    ConfigurationUpdater(){}
    ~ConfigurationUpdater(){}

    void start(const string& addr, IConfigurationUpdater* callback) {
        thread_loop_ = new std::thread(&ConfigurationUpdater::_run, this, addr, callback);
    }

private:
    void _run(const string& addr, IConfigurationUpdater* callback) 
    {        
        Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = addr;
        props[PropertyKeyConst::NAMESPACE] = "bcts";
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        try {
            NacosListener *listener1 = new NacosListener(n, "riskcontrol", "symbols", callback);
            n->addListener("symbols", "riskcontrol", listener1);
        }
        catch (NacosException &e) {
            _log_and_print("[fatal] Request failed with curl code:%d Reason:%s", e.errorcode(), e.what());
            return;
        }

        while( true ) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};