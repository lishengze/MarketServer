#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
using namespace std;

struct QuoteConfiguration
{
    double MakerFee; // 百分比；maker手续费
    double TakerFee; // 百分比；taker手续费
    int PriceBias; // 百分比；价格偏移量（买卖盘相同）
    int VolumeBias; // 百分比；单量偏移量（买卖盘相同）
    int UserPercent; // 百分比；用户账户可动用比例
    int HedgePercent; // 百分比；对冲账户可动用比例
};

class IConfigurationUpdater {
public:
    virtual void on_configuration_update(const QuoteConfiguration& config) = 0;
};

class ConfigurationUpdater {
public:
    ConfigurationUpdater(){}
    ~ConfigurationUpdater(){}

    void start(IConfigurationUpdater* callback) {
        thread_loop_ = new std::thread(&ConfigurationUpdater::_run, this, callback);
    }

private:
    void _run(IConfigurationUpdater* callback) {
        while( true ) {
            QuoteConfiguration config;
            config.MakerFee = 0.2;
            config.TakerFee = 0.2;
            config.PriceBias = 10;
            config.VolumeBias = 10;
            config.UserPercent = 50;
            config.HedgePercent = 50;
            callback->on_configuration_update(config);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};