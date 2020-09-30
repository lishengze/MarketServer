#pragma once

#include "pandora/util/singleton.hpp"
#include "pandora/util/path_util.h"
#include "pandora/messager/ut_log.h"
#include <string>
#include <iostream>
#include <fstream>
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
            json js = json::parse(contents);

            // grpc
            grpc_push_addr_ = js["grpc"]["push_addr"].get<string>();

            // debug
            sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
            dump_binary_only_ = bool(js["debug"]["dump_binary_only_"].get<int>());

            // redis quote
            quote_redis_host_ = js["redis_quote"]["host"].get<string>();
            quote_redis_port_ = js["redis_quote"]["port"].get<int>();
            quote_redis_password_ = js["redis_quote"]["password"].get<string>();
            quote_redis_snap_interval_ = js["redis_quote"]["snap_interval"].get<int>();

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
    // grpc push address
    string grpc_push_addr_;

    // [for debug] sample symbol
    string sample_symbol_;
    // [for debug] only dump binary data instead of push
    bool dump_binary_only_;

    // quote redis config
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;
    int quote_redis_snap_interval_;

    // logger
    UTLogPtr logger_;
};