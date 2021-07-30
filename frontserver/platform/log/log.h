#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/util/time_util.h"

#include "hub_interface.h"
#include "hub_struct.h"
#include "../data_structure/base_data.h"

using std::string;
using std::cout;
using std::endl;

class Log
{
    public:
        Log()
        {
            log_file_.open(file_name_, ios_base::ate | ios_base::out);
        }

        ~Log();

        void log(string msg, string flag)
        {
            std::lock_guard<std::mutex> lk(mutex_);
            
            cout << utrade::pandora::SecTimeStr() << " " << flag << ": " << msg << endl;

            // if (!log_file_.is_open())
            // {
            //     log_file_.open(file_name_, ios_base::ate | ios_base::out | ios_base::app);
            // }

            // log_file_ << flag << ": " << msg << "\n";
            // log_file_.close();
        }

        void start();

        void record_input_info(const string& channel);

        void record_input_info(const string& channel, const SDepthData& quote);

        void record_input_info(const string& channel, const Trade& trade);

        void record_input_info(const string& channel, const vector<KlineData>& klines);        

        void record_output_info(const string& channel, const SDepthData& quote);

        void record_output_info(const string& channel, const Trade& trade);

        void record_output_info(const string& channel, const vector<KlineData>& klines);

        void record_output_info(const string& info);

        void statistic_thread_main();

        void print_statistic_data();

        void print_input_info();

        void print_output_info();

    private:



        int                             statistic_secs_{10};
        boost::shared_ptr<std::thread>  statistic_thread_{nullptr};

        string                          last_statistic_time_str_;

        map<string, int>                input_statistic_map_;
        std::mutex                      input_statistic_map_mutex_;

        map<string, int>                output_statistic_map_;
        std::mutex                      output_statistic_map_mutex_;


    private:
        string              file_name_{"server.log"};
        std::ofstream       log_file_;
        std::mutex          mutex_;
};

#define LOG utrade::pandora::ThreadSafeSingleton<Log>::DoubleCheckInstance()

#define LOG_ERROR(msg) LOG->log(msg, "Error")
#define LOG_INFO(msg) LOG->log(msg, "Info ")
#define LOG_DEBUG(msg) LOG->log(msg, "Debug")