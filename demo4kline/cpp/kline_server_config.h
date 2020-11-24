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
#include "base/cpp/basic.h"

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
        logger_ = utrade::pandora::UTLog::getStrategyLogger("RiskController", "RiskController");

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

            // redis quote
            quote_redis_host_ = js["redis_quote"]["host"].get<string>();
            quote_redis_port_ = js["redis_quote"]["port"].get<int>();
            quote_redis_password_ = js["redis_quote"]["password"].get<string>();

            // endpoint
            endpoint_addr_ = js["endpoint"]["addr"].get<string>();
            
            // server
            for (auto iter = js["server"]["symbols"].begin(); iter != js["server"]["symbols"].end(); ++iter) {
                const string& symbol = *iter;
                include_symbols_.insert(symbol);
            }
            for (auto iter = js["server"]["exchanges"].begin(); iter != js["server"]["exchanges"].end(); ++iter) {
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

public:
    // endpoint
    string endpoint_addr_;

    // server
    std::set<string> include_symbols_;      // 包含的品种 CFG_INCLUDE_SYMBOLS
    std::set<string> include_exchanges_;    // 包含的交易所 CFG_INCLUDE_EXCHANGES

    // redis
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;

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
