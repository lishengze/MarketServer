#pragma once

#include "pandora/util/singleton.hpp"
#include "pandora/util/path_util.h"
#include "pandora/util/json.hpp"
#include "pandora/messager/ut_log.h"

#include "base/cpp/tinyformat.h"
#include "global_declare.h"
#include "Log/log.h"

#include <sstream>

using njson = nlohmann::json;



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
            // read the config file
            std::ifstream in_config(file_name);
            std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
            std::cout << contents << std::endl;

            LOG_INFO(file_name);
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


            if (js["account_risk_ctrl_open"].is_null())
            {
                account_risk_ctrl_open_     = js["account_risk_ctrl_open"].get<bool>();
            }
            else
            {
                LOG_ERROR("Need account_risk_ctrl_open");
            }

            if (js["order_risk_ctrl_open"].is_null())
            {
                order_risk_ctrl_open_       = js["order_risk_ctrl_open"].get<bool>();
            }
            else
            {
                LOG_ERROR("Need order_risk_ctrl_open");
            }

            if (js["bias_risk_ctrl_open"].is_null())
            {
                bias_risk_ctrl_open_        = js["bias_risk_ctrl_open"].get<bool>();
            }
            else
            {
                LOG_ERROR("Need bias_risk_ctrl_open");
            }

            if (js["watermark_risk_ctrl_open"].is_null())
            {
                watermark_risk_ctrl_open_   = js["watermark_risk_ctrl_open"].get<bool>();
            }
            else
            {
                LOG_ERROR("Need watermark_risk_ctrl_open");
            }

            if (js["pricesion_risk_ctrl_open"].is_null())
            {
                pricesion_risk_ctrl_open_   = js["pricesion_risk_ctrl_open"].get<bool>();
            }
            else
            {
                LOG_ERROR("Need pricesion_risk_ctrl_open");
            }

            if (js["check_symbol_secs"].is_null())
            {
                check_symbol_secs = js["check_symbol_secs"].get<int>();
            }
            else
            {
                LOG_ERROR("Need check_symbol_secs");
            }          

            if (!js["test_symbol"].is_null())
            {
                test_symbol = js["test_symbol"].get<string>();
            }

            LOG_INFO(str());
        }
        catch (std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    }

    string str() 
    {
        string result;
        stringstream s_s;
        s_s << "grpc_quote_addr_:   " << grpc_quote_addr_ << "\n"
            << "grpc_account_addr_: " << grpc_account_addr_ << "\n"
            << "grpc_publish_addr_: " << grpc_publish_addr_ << "\n"
            << "nacos_addr_:        " << nacos_addr_ << "\n"
            << "nacos_namespace_:   " << nacos_namespace_ << "\n"
            << "sample_symbol_:     " << sample_symbol_ << "\n"
            << "dump_binary_only_:  " << dump_binary_only_ << "\n"
            << "output_to_screen_:  " << output_to_screen_ << "\n"
            << "account_risk_ctrl_open_: " << account_risk_ctrl_open_ << "\n"
            << "order_risk_ctrl_open_:   " << order_risk_ctrl_open_ << "\n"
            << "test_symbol:        " << test_symbol << "\n"
            << "check_symbol_secs:  " << check_symbol_secs << "\n";
        return s_s.str();
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

    bool account_risk_ctrl_open_{false};
    bool order_risk_ctrl_open_{false};
    bool bias_risk_ctrl_open_{false};
    bool watermark_risk_ctrl_open_{false};
    bool pricesion_risk_ctrl_open_{false};

    string test_symbol;
    int     check_symbol_secs{5};

    // logger
    UTLogPtr logger_;

    
};


#define CONFIG utrade::pandora::ThreadSafeSingleton<Config>::DoubleCheckInstance()
#define ACCOUNT_RISKCTRL_OPEN CONFIG->account_risk_ctrl_open_
#define ORDER_RISKCTRL_OPEN CONFIG->order_risk_ctrl_open_
#define BIAS_RISKCTRL_OPEN CONFIG->bias_risk_ctrl_open_
#define WATERMARK_RISKCTRL_OPEN CONFIG->watermark_risk_ctrl_open_
#define PRICESION_RISKCTRL_OPEN CONFIG->pricesion_risk_ctrl_open_

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
