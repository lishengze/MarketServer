#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"

class INacosCallback {
public:
    virtual void on_config_channged(const Document& symbols) = 0;
};

class ConfigurationClient: public NacosClient
{
public:
    void set_callback(INacosCallback* callback) { callback_ = callback; }
    
    // derive from NacosClient
    void config_changed(const string& group, const string& dataid, const NacosString &configInfo);
private:
    // derive from NacosClient
    void _run();

    INacosCallback* callback_ = nullptr;

    // NacosListener
    NacosListener symbol_watcher_;
    NacosListener hedger_watcher_;
    NacosListener risk_watcher_;

    // 处理配置数据
    void _parse_config();
    NacosString hedge_params_;
    NacosString symbol_params_;
    NacosString risk_params_; // for 品种更新频率 和 品种更新深度
};
