#pragma once

#include "global_declare.h"
#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"
#include "risk_controller_define.h"
#include "pandora/util/thread_safe_singleton.hpp"

#include "data_struct/data_struct.h"



class IConfigurationUpdater {
public:
    virtual void on_configuration_update(const map<TSymbol, MarketRiskConfig>& config) = 0;

    virtual void on_configuration_update(const map<TSymbol, SymbolConfiguration>& config) = 0;

    virtual void on_configuration_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config) = 0;
};

class ConfigurationClient: public NacosClient
{
public:
    void set_callback(IConfigurationUpdater* callback) { callback_ = callback; }
    
    // derive from NacosClient
    void config_changed(const std::string& group, const std::string& dataid, const NacosString &configInfo);

    void load_market_risk(const NacosString &configInfo);

    void load_symbol_params(const NacosString &configInfo);

    void load_hedge_params(const NacosString &configInfo);
    
    bool check_symbol(std::string symbol);

private:
    // derive from NacosClient
    void _run();

    IConfigurationUpdater* callback_ = nullptr;

    // NacosListener
    NacosListener risk_watcher_;
    NacosListener symbol_watcher_;    
    NacosListener hedger_watcher_;

    SymbolConfiguration symbol_params_;


    // 处理配置数据
    void _parse_config();
    NacosString risk_params_; // for 品种更新频率 和 品种更新深度

    NacosString hedge_params_;

    // map<TSymbol, MarketRiskConfig> risk_config_;
    // std::mutex                       risk_config_mutex_;
};

#define RISK_CONFIG utrade::pandora::ThreadSafeSingleton<ConfigurationClient>::DoubleCheckInstance() 
