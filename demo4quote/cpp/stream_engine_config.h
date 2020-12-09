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


class Config
{
public:
    Config(){}
    virtual ~Config(){}

    // pasrse utrade.config.json file
    void parse_config(const std::string& file_name);

    string get_config() const;
    void set_config(const string& config);

public:
    // grpc
    string grpc_publish_addr_;          // grpc服务发布地址

    // 服务配置
    string sample_symbol_;              // 采样品种
    bool publish_data_;                 // 是否发布行情更新
    bool dump_binary_;                  // 是否保存原始二进制数据
    bool output_to_screen_;             // 是否打印采样信息到屏幕
    bool replay_mode_;                   // 回放模式
    unsigned int replay_ratio_;                  // 回放速度：倍速

    // redis行情源地址
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;
    int quote_redis_snap_interval_;

    // nacos配置服务器地址
    string nacos_addr_;

    // nacos配置信息缓存
    mutable std::mutex mutex_faw_config_;   
    string faw_config_;
    
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
