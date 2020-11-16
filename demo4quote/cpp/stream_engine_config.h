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
    void parse_config(const std::string& file_name) {
        // init logger
        logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

        try
        {
            // get the running module path
            //string WorkFolder = utrade::pandora::get_module_path();
            // append the prefix
            //string intact_file_name = WorkFolder + file_name;
            UT_LOG_INFO(logger_, "System Parse Config File " << file_name);
            // read the config file
            std::ifstream in_config(file_name);
            std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
            // std::cout << contents << std::endl;
            njson js = njson::parse(contents);

            // grpc
            grpc_publish_addr_ = js["grpc"]["publish_addr"].get<string>();
            grpc_publish_depth_ = js["grpc"]["publish_depth"].get<int>();;
            grpc_publish_frequency_ = js["grpc"]["frequency"].get<int>();
            grpc_publish_raw_frequency_ = js["grpc"]["raw_frequency"].get<int>();

            // system
            sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
            publish_data_ = bool(js["debug"]["publish_data"].get<int>());
            dump_binary_ = bool(js["debug"]["dump_binary"].get<int>());
            output_to_screen_ = bool(js["debug"]["output_to_screen"].get<int>());
            replay_mode_ = bool(js["debug"]["replay_mode"].get<int>());
            replay_ratio_ = js["debug"]["replay_ratio"].get<int>();

            // redis quote
            quote_redis_host_ = js["redis_quote"]["host"].get<string>();
            quote_redis_port_ = js["redis_quote"]["port"].get<int>();
            quote_redis_password_ = js["redis_quote"]["password"].get<string>();
            quote_redis_snap_interval_ = js["redis_quote"]["snap_interval"].get<int>();

            // config center
            nacos_addr_ = js["nacos"]["addr"].get<string>();
            for (auto iter = js["include"]["symbols"].begin(); iter != js["include"]["symbols"].end(); ++iter) {
                const string& symbol = *iter;
                include_symbols_.insert(symbol);
            }
            for (auto iter = js["include"]["exchanges"].begin(); iter != js["include"]["exchanges"].end(); ++iter) {
                const string& exchange = *iter;
                include_exchanges_.insert(exchange);
            }                
            UT_LOG_INFO(logger_, "Parse Config finish.");
        }
        catch (std::exception& e)
        {
            std::cerr << "Config parse exception: " << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "Config Unknown Exception! " << std::endl;
        }
    }

    int get_precise(const string& symbol) const {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        auto iter = symbol_precise_.find(symbol);
        if( iter == symbol_precise_.end() ) {
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

    void set_configuration_precise(const std::unordered_map<string, int>& vals)
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        symbol_precise_ = vals;
    }

    void set_configuration_fee(const std::unordered_map<string, std::unordered_map<string, SymbolFee>>& vals)
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        symbol_fee_ = vals;
    }
public:
    // grpc
    string grpc_publish_addr_;          // grpc服务发布地址
    int grpc_publish_frequency_;        // 品种聚合后的更新频率：每秒frequency次
    int grpc_publish_depth_;            // 品种聚合后的深度
    int grpc_publish_raw_frequency_;    // 品种原始行情发布频率：每秒frequency次

    // system
    string sample_symbol_;              // 采样品种
    bool publish_data_;                 // 是否发布行情更新
    bool dump_binary_;                  // 是否保存原始二进制数据
    bool output_to_screen_;             // 是否打印采样信息到屏幕
    bool replay_mode_;                   // 回放模式
    int replay_ratio_;                  // 回放速度：倍速

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

#define _log_and_print(...)                                             \
    do {                                                                \
        UT_LOG_INFO_FMT(CONFIG->logger_, __VA_ARGS__);                  \
        _println_(__VA_ARGS__);                                         \
    } while(0)                                                          
