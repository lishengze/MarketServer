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

#define PROG_VERSION "1.0.0.1"

#define CONFIG utrade::pandora::Singleton<Config>::GetInstance()
#define KAFKA_CONFIG CONFIG->kafka_config_

using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;


struct KafkaConfig
{
    string                  bootstrap_servers;
    std::list<string>       sub_topic_list;
};

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
    string mode_;                       // realtime | replay
    string sample_symbol_;              // 采样品种
    unsigned int replay_ratio_;         // 回放速度：倍速
    unsigned int replay_replicas_;      // 复制行情，测试容量
    bool dump_;                         // 是否保存原始二进制数据

    // redis行情源地址
    string quote_redis_host_;
    int quote_redis_port_;
    string quote_redis_password_;
    int quote_redis_snap_interval_;

    // nacos配置服务器地址
    string nacos_addr_;
    string nacos_namespace_;

    // nacos配置信息缓存
    mutable std::mutex mutex_faw_config_;   
    string faw_config_;

    // reporter
    string reporter_addr_;
    string reporter_port_;
    
    // logger
    UTLogPtr logger_;

    KafkaConfig         kafka_config_;

    type_tick GLOBAL_HUOBI_BTC;
    type_tick GLOBAL_BINANCE_BTC;
    type_tick GLOBAL_OKEX_BTC;
    type_tick GLOBAL_BCTS_BTC;
};


#define _log_and_print(fmt, ...)                                     \
    do {                                                             \
        std::string log_msg;                                         \
        try {                                                        \
            log_msg = tfm::format(fmt, ##__VA_ARGS__);               \
        } catch (tinyformat::format_error& fmterr) {                 \
            /* Original format string will have newline so don't add one here */ \
            log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt; \
        }                                                            \
        std::string new_fmt = "%s:%d - " + string(fmt);              \
                                                                     \
        tfm::printfln("%s:%d - %s", __FILE__, __LINE__, log_msg);    \
                                                                     \
        UT_LOG_INFO_FMT(CONFIG->logger_, "%s:%d - %s", __FILE__, __LINE__, log_msg.c_str());\
    } while(0)                                                          
