#pragma once

#include "pandora/util/singleton.hpp"
#include "pandora/util/path_util.h"
#include "pandora/util/json.hpp"
#include "pandora/messager/ut_log.h"
using njson = nlohmann::json;
#include <string>
#include <iostream>
#include <fstream>
#include <set>
using namespace std;
#include "stream_engine_define.h"

#define CONFIG utrade::pandora::Singleton<Config>::GetInstance()

using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;


struct SymbolFee
{
    int fee_type;       // 0表示fee不需要处理（默认），1表示fee值为比例，2表示fee值为绝对值
    double maker_fee;
    double taker_fee;

    SymbolFee() {
        fee_type = 0;
        maker_fee = taker_fee = 0.2;
    }

    void compute(const SDecimal& src, SDecimal& dst, bool is_ask) const
    {
        if( fee_type == 1 ) {
            if( is_ask ) {
                dst = src + maker_fee;
            } else {
                dst = src - taker_fee;
            }
        } else if( fee_type == 2 ) {
            if( is_ask ) {
                dst = src * (100 + maker_fee) / 100.0;
            } else {
                dst = src * (100 - taker_fee) / 100.0;
            }
        } else {
            dst = src;
        }
    }
};

class Config
{
public:
    Config(){}
    virtual ~Config(){}

    // pasrse utrade.config.json file
    void parse_config(const std::string& file_name);

    int get_precise(const string& symbol) const {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        auto iter = symbol_precise_.find(symbol);
        if( iter == symbol_precise_.end() ) {
            cout << "cannot find precise " << symbol << endl;
            return -1;
        }
        return iter->second;
    }

    SymbolFee get_fee(const string& exchange, const string& symbol) {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        return symbol_fee_[exchange][symbol];
    }

    bool is_forbidden_exchage(const TExchange& exchange) {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        return forbidden_exchanges_.find(exchange) != forbidden_exchanges_.end();
    }

    void set_configuration_precise(const std::unordered_map<string, int>& vals);

    void set_configuration_fee(const std::unordered_map<string, std::unordered_map<string, SymbolFee>>& vals)
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        symbol_fee_ = vals;
    }
public:
    // grpc
    string grpc_publish_addr_;          // grpc服务发布地址
    float grpc_publish_frequency_;        // 品种聚合后的更新频率：每秒frequency次
    unsigned int grpc_publish_depth_;            // 品种聚合后的深度
    float grpc_publish_raw_frequency_;    // 品种原始行情发布频率：每秒frequency次

    // system
    string sample_symbol_;              // 采样品种
    bool publish_data_;                 // 是否发布行情更新
    bool dump_binary_;                  // 是否保存原始二进制数据
    bool output_to_screen_;             // 是否打印采样信息到屏幕
    bool replay_mode_;                   // 回放模式
    unsigned int replay_ratio_;                  // 回放速度：倍速

    // redis quotation source
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;
    int quote_redis_snap_interval_;

    // from database
    std::set<string> include_symbols_;      // 包含的品种 CFG_INCLUDE_SYMBOLS
    std::set<string> include_exchanges_;    // 包含的交易所 CFG_INCLUDE_EXCHANGES
    std::set<string> forbidden_exchanges_;  // 设置（临时）关闭的交易所 CFG_FORBIDDEN_EXCHANGES

    // from config center
    string nacos_addr_;
    mutable std::mutex mutex_configuration_;
    std::unordered_map<string, int> symbol_precise_;    // 各品种统一精度 CFG_SYMBOL_PRECISE
    std::unordered_map<string, std::unordered_map<string, SymbolFee>> symbol_fee_;  // 各品种手续费 CFG_SYMBOL_FEE

    // logger
    UTLogPtr logger_;
};

#define _log_and_print2(...)                                            \
    do {                                                                \
        UT_LOG_INFO_FMT(CONFIG->logger_, __VA_ARGS__);                  \
        _println_(__VA_ARGS__);                                         \
    } while(0)                                                          

#define _log_and_print(format, ...)                                     \
    do {                                                                \
        _log_and_print2("%s:%d - " format, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)                                                          
