#pragma once
#include "../pandora_declare.h"

PANDORA_NAMESPACE_START

// 合约属性列表
struct InstrumentProperty
{
    string ExchangeID;      // 交易所 ID 编号
    string InstrumentID;    // 合约 ID 编号
    bool AcceptShort;       // 是否支持裸空机制
    string BaseCurrency;    // 基础交易货币
    double MarginRate;      // 保证金比率
    double Multiple;        // 合约乘数
    double MakerFee;        // maker 手续费
    double TakerFee;        // taker 手续费
    double FlatFee;         // 常规手续费
    string ModeFee;         // 收取手续费模式
    double VolumePrecise;   // 报单量精度
    double PricePrecise;    // 价格精度
    double MinimumQuota;    // 最小交易额
    double MaximumVolume;   // 最大交易量
    bool   ClassicSettle;   // 常规处理
};
DECLARE_PTR(InstrumentProperty);

// 合约属性集合
struct InstrumentProperties
{
    void append_property(InstrumentPropertyPtr property)
    {
        Properties.emplace(property->ExchangeID+property->InstrumentID, property);
    }
    InstrumentPropertyPtr get_instrument_property(const string& exchangeid, const string& instrumentid)
    {
        auto iter_find_inst = Properties.find(exchangeid+instrumentid);
        if (iter_find_inst!=Properties.end())
        {
            return iter_find_inst->second;
        }
        return nullptr;

    }
    std::unordered_map<string, InstrumentPropertyPtr> Properties;
};
DECLARE_PTR(InstrumentProperties);

PANDORA_NAMESPACE_END