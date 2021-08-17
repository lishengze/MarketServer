#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <log4cplus/log4cplus.h>

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/util/time_util.h"

#include "hub_interface.h"
#include "hub_struct.h"
#include "../data_structure/base_data.h"
#include "../front_server_declare.h"


using std::string;
using std::cout;
using std::endl;

class Log
{
    public:
        Log();
        
        ~Log();

        void log(string info, string flag)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            
            cout << utrade::pandora::SecTimeStr() << " " << flag << ": " << info << endl;

            // if (!log_file_.is_open())
            // {
            //     log_file_.open(file_name_, ios_base::ate | ios_base::out | ios_base::app);
            // }

            // log_file_ << flag << ": " << info << "\n";
            // log_file_.close();
        }

        void init_logger();

        void start();

        void record_input_info(const string& channel);

        void record_input_info(const string& channel, const SDepthData& quote);

        void record_input_info(const string& channel, const Trade& trade);

        void record_input_info(const string& channel, const vector<KlineData>& klines);        

        void record_output_info(const string& channel, const SDepthData& quote);

        void record_output_info(const string& channel, const Trade& trade);

        void record_output_info(const string& channel, const vector<KlineData>& klines);

        void record_output_info(const string& channel, const std::vector<KlineDataPtr>& klines);

        void record_output_info(const string& channel, const string& details);

        void record_output_info(const string& info);

        void record_client_info(const string& client_id, const string& info);

        void statistic_thread_main();

        void print_statistic_data();

        void print_input_info();

        void print_output_info();

        void print_client_info();

        void log_trace(const string& info);
        void log_debug(const string& info);
        void log_info(const string& info);
        void log_warn(const string& info);
        void log_error(const string& info);
        void log_fatal(const string& info);

        void log_client_request(const string& info);
        void log_client_response(const string& info);

        void log_source_input(const string& info);
        void log_exception(const string& info);

    private:

        int                             statistic_secs_{10};
        boost::shared_ptr<std::thread>  statistic_thread_{nullptr};

        string                          last_statistic_time_str_;

        map<string, int>                input_statistic_map_;
        std::mutex                      input_statistic_map_mutex_;

        map<string, int>                output_statistic_map_;
        std::mutex                      output_statistic_map_mutex_;

        std::map<string, std::vector<std::string>> client_info_map_;      
        std::mutex                      client_info_map_mutex_; 


    private:
        string              file_name_{"server.log"};
        std::ofstream       log_file_;
        std::mutex          mutex_;

    
    private:
        boost::shared_ptr<log4cplus::Logger>        common_logger_;

        log4cplus::SharedAppenderPtr                trace_appender_;
        

        log4cplus::SharedAppenderPtr                info_appender_;
        log4cplus::SharedAppenderPtr                warn_appender_;

        boost::shared_ptr<log4cplus::Logger>        debug_logger_;      
        log4cplus::SharedAppenderPtr                debug_appender_;  

        boost::shared_ptr<log4cplus::Logger>        client_request_logger_;
        log4cplus::SharedAppenderPtr                client_request_appender_;
        
        boost::shared_ptr<log4cplus::Logger>        client_response_logger_;
        log4cplus::SharedAppenderPtr                client_response_appender_;

        boost::shared_ptr<log4cplus::Logger>        source_input_logger_;
        log4cplus::SharedAppenderPtr                source_input_appender_;
};

#define LOG utrade::pandora::ThreadSafeSingleton<Log>::DoubleCheckInstance()

// #define LOG_ERROR(info) LOG->log(info, "Error")
// #define LOG_INFO(info) LOG->log(info, "Info ")
// #define LOG_DEBUG(info) LOG->log(info, "Debug")

static inline string get_simple_file_name(string path) {
    string::size_type i = path.find_last_of("/") + 1;
    return path.substr(i, path.size()-i);
}

#define LOG_HEADER get_simple_file_name(string(__FILE__)) + ":" + string(__FUNCTION__) + "." + std::to_string(__LINE__) + " " 

#define LOG_TRACE(info) LOG->log_trace(LOG_HEADER + info)
#define LOG_DEBUG(info) LOG->log_debug(LOG_HEADER + info)
#define LOG_INFO(info) LOG->log_info(LOG_HEADER + info)
#define LOG_WARN(info) LOG->log_warn(LOG_HEADER + info)
#define LOG_ERROR(info) LOG->log_error(LOG_HEADER + info)
#define LOG_FATAL(info) LOG->log_fatal(LOG_HEADER + info)

#define LOG_CLIENT_REQUEST(info) LOG->log_client_request(LOG_HEADER + info)
#define LOG_CLIENT_RESPONSE(info) LOG->log_client_response(LOG_HEADER + info)
#define LOG_SOURCE_INPUT(info) LOG->log_source_input(LOG_HEADER + info)
