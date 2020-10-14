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

#define CONFIG utrade::pandora::Singleton<Config>::GetInstance()

using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
class Config
{
public:
    Config(){}
    virtual ~Config(){}

    // pasrse utrade.config.json file
    void parse_config(const std::string& file_name) {
        // init logger
        logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

        // init precise
        symbol_precise_["BTC_USDT"] = 1;  // 按照okex的1位小数点来

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
            grpc_push_addr_ = js["grpc"]["push_addr"].get<string>();
            grpc_push_depth_ = 10;
            frequency_ = js["grpc"]["frequency"].get<int>();

            // debug
            sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
            publish_data_ = bool(js["debug"]["publish_data"].get<int>());
            dump_binary_ = bool(js["debug"]["dump_binary"].get<int>());
            output_to_screen_ = bool(js["debug"]["output_to_screen"].get<int>());

            // redis quote
            quote_redis_host_ = js["redis_quote"]["host"].get<string>();
            quote_redis_port_ = js["redis_quote"]["port"].get<int>();
            quote_redis_password_ = js["redis_quote"]["password"].get<string>();
            quote_redis_snap_interval_ = js["redis_quote"]["snap_interval"].get<int>();

            // include
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
        auto iter = symbol_precise_.find(symbol);
        if( iter == symbol_precise_.end() ) {
            return -1;
        }
        return iter->second;
    }
public:
    // grpc push
    int frequency_;     // 品种聚合后的更新频率：每秒frequency_次
    string grpc_push_addr_;
    int grpc_push_depth_;

    // [for debug] sample symbol
    string sample_symbol_;
    // [for debug] only dump binary data instead of push
    bool publish_data_;
    bool dump_binary_;
    // [for debug] output to screen
    bool output_to_screen_;

    // quote redis config
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;
    int quote_redis_snap_interval_;

    // include
    std::set<string> include_symbols_;
    std::set<string> include_exchanges_;

    // precise configuration
    std::unordered_map<string, int> symbol_precise_;
    // logger
    UTLogPtr logger_;
};