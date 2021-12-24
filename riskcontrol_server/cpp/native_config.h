#pragma once

#include "pandora/util/singleton.hpp"
#include "pandora/util/path_util.h"
#include "pandora/util/json.hpp"
#include "pandora/messager/ut_log.h"

#include "base/cpp/tinyformat.h"
#include "base/cpp/basic.h"
#include "Log/log.h"

using njson = nlohmann::json;

class NativeConfig
{
public:
    NativeConfig(){}
    virtual ~NativeConfig(){}

    // pasrse utrade.config.json file
    void parse_config(const std::string& file_name) {

        try
        {
            // get the running module path
            //string WorkFolder = utrade::pandora::get_module_path();
            // append the prefix
            //string intact_file_name = WorkFolder + file_name;

            LOG_INFO("System Parse NativeConfig File " + file_name);

            // read the config file
            std::ifstream in_config(file_name);
            std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
            LOG_INFO(contents);
            
            njson js = njson::parse(contents);

            // grpc
            grpc_quote_addr_ = js["grpc"]["quote_addr"].get<string>();
            grpc_account_addr_ = js["grpc"]["account_addr"].get<string>();
            grpc_publish_addr_ = js["grpc"]["publish_addr"].get<string>();

            // nacos
            nacos_addr_ = js["nacos"]["addr"].get<string>();
            nacos_namespace_ = js["nacos"]["namespace"].get<string>();
            
            // debug
            sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
            dump_binary_only_ = bool(js["debug"]["dump_binary_only"].get<int>());
            output_to_screen_ = bool(js["debug"]["output_to_screen"].get<int>());
        }
        catch (std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

public:
    // grpc quote
    string grpc_quote_addr_;
    string grpc_account_addr_;
    string grpc_publish_addr_;

    // nacos
    string nacos_addr_;
    string nacos_namespace_;

    // [for debug] sample symbol
    string sample_symbol_;
    // [for debug] only dump binary data instead of push
    bool dump_binary_only_;
    // [for debug] output to screen
    bool output_to_screen_;

};

#define NATIVE_CONFIG utrade::pandora::ThreadSafeSingleton<NativeConfig>::DoubleCheckInstance()