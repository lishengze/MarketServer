#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"
#include "risk_controller_define.h"

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

class ConfigurationClient: public NacosClient
{
public:
    void set_callback(IConfigurationUpdater* callback) { callback_ = callback; }
    
    // derive from NacosClient
    void config_changed(const string& group, const string& dataid, const NacosString &configInfo);
private:
    // derive from NacosClient
    void _run();

    IConfigurationUpdater* callback_ = nullptr;

    // NacosListener
    NacosListener risk_watcher_;

    // 处理配置数据
    void _parse_config();
    NacosString risk_params_; // for 品种更新频率 和 品种更新深度
};
