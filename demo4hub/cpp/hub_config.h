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
    void parse_config(const std::string& file_name);

public:
    // grpc
    string risk_controller_addr_;          // grpc服务发布地址
    string stream_engine_addr_;

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
