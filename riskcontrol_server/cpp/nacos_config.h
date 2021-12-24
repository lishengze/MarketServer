#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"


#include "pandora/util/thread_safe_singleton.hpp"

#include "data_struct/data_struct.h"

class NacosConfig: public NacosClient
{
public:
    void set_callback(IConfigurationUpdater* callback) { callback_ = callback; }
    
    // derive from NacosClient
    void config_changed(const std::string& group, const std::string& dataid, const NacosString &configInfo);

    void load_symbol_config(const NacosString &configInfo);

    void load_hedge_config(const NacosString &configInfo);

    void load_risk_config(const NacosString &configInfo);
private:
    // derive from NacosClient
    void _run();

    IConfigurationUpdater* callback_ = nullptr;

    // NacosListener
    NacosListener risk_watcher_;
    NacosListener symbol_watcher_;    
    NacosListener hedger_watcher_;
};

#define NACOS_CONFIG utrade::pandora::ThreadSafeSingleton<NacosConfig>::DoubleCheckInstance() 
