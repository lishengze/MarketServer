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

            // grpc
            grpc_quote_addr_ = js["grpc"]["quote_addr"].get<string>();
            grpc_account_addr_ = js["grpc"]["account_addr"].get<string>();
            grpc_publish_addr_ = js["grpc"]["publish_addr"].get<string>();
            grpc_publish_depth_ = js["grpc"]["publish_depth"].get<int>();

            // debug
            sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
            dump_binary_only_ = bool(js["debug"]["dump_binary_only"].get<int>());
            output_to_screen_ = bool(js["debug"]["output_to_screen"].get<int>());
            
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
    // grpc quote
    string grpc_quote_addr_;
    string grpc_account_addr_;
    string grpc_publish_addr_;
    int grpc_publish_depth_;

    // [for debug] sample symbol
    string sample_symbol_;
    // [for debug] only dump binary data instead of push
    bool dump_binary_only_;
    // [for debug] output to screen
    bool output_to_screen_;

    // logger
    UTLogPtr logger_;
};

#define _log_and_print(...)                                             \
    do {                                                                \
        UT_LOG_INFO_FMT(CONFIG->logger_, __VA_ARGS__);                  \
        _println_(__VA_ARGS__);                                         \
    } while(0)                                                          
