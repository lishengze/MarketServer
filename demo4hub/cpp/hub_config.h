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

#include "Log/log.h"

#define CONFIG utrade::pandora::Singleton<Config>::GetInstance()

using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;

class Config
{
public:
    Config(){

        string work_dir = utrade::pandora::get_module_path() + "/build/";
        string program_name = "source_data";

        cout << "work_dir: " << work_dir << "\n"
            << "program_name: " << program_name << "\n"
            << endl;

        LOG->set_work_dir(work_dir);
        LOG->set_program_name(program_name);
        LOG->start();
    }
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
        tfm::printfln("%s:%d - %s", __func__, __LINE__, log_msg);    \
                                                                     \
        UT_LOG_INFO_FMT(CONFIG->logger_, "%s:%d - %s", __func__, __LINE__, log_msg.c_str());\
    } while(0)                                                          
