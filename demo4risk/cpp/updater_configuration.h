#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"
#include "risk_controller_define.h"

#include "pandora/util/thread_safe_singleton.hpp"

struct QuoteConfiguration
{
    //double MakerFee; // 百分比；maker手续费
    //double TakerFee; // 百分比；taker手续费
    string symbol;
    uint32 PublishFrequency;
    uint32 PublishLevel;
    uint32 PriceOffsetKind;  // 价格偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double PriceOffset;      // 百分比；价格偏移量（买卖盘相同）

    uint32 AmountOffsetKind; // 数量偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double AmountOffset;     // 行情数量偏移

    double DepositFundRatio; // 充值资金动用比例

    //double UserPercent;    // 百分比；用户账户可动用比例
    double HedgeFundRatio;   // 百分比；对冲账户可动用比例

    uint32 OTCOffsetKind;    // 询价偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.

    double OtcOffset;        // 询价偏移
    bool   IsPublish{true};

    string desc() const {
        return tfm::format("symbol = %s, PublishFrequency: %d, PublishLevel:%d, PriceOffsetKind:%d, PriceOffset: %f, \
                            AmountOffsetKind:%d, AmountOffset:%d, DepositFundRatio:%.2f, HedgeFundRatio=%.2f, OTCOffsetKind=%d, \
                            OtcOffset:%.2f, IsPublish:%d", 
                            symbol.c_str(), PublishFrequency, PublishLevel, PriceOffsetKind, PriceOffset,
                            AmountOffsetKind, AmountOffset, DepositFundRatio, HedgeFundRatio, OTCOffsetKind,
                            OtcOffset, IsPublish);
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

    bool check_symbol(string symbol);

    map<TSymbol, QuoteConfiguration>& get_risk_config() { return risk_config_;}


private:
    // derive from NacosClient
    void _run();

    IConfigurationUpdater* callback_ = nullptr;

    // NacosListener
    NacosListener risk_watcher_;

    // 处理配置数据
    void _parse_config();
    NacosString risk_params_; // for 品种更新频率 和 品种更新深度

    map<TSymbol, QuoteConfiguration> risk_config_;
};

#define RISK_CONFIG utrade::pandora::ThreadSafeSingleton<ConfigurationClient>::DoubleCheckInstance() 
