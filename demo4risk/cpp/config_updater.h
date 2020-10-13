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
    double MakerFee;
    double TakerFee;
    int PriceBias; // percent
    int VolumeBias; // percent
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
            callback->on_configuration_update(config);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};