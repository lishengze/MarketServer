#pragma once

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/base_log.h"
#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

class CommLog: public BaseLog 
{
public:
    virtual void log_info_(const string& info)
    {
        LOG4CPLUS_INFO(*common_logger_.get(), info);

        cout << info << endl;
    }

    void record_input_depth(const string& info, const SDepthQuote& quote)
    {
        try
        {
            BaseLog::record_input_info(info);
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }

    void record_input_trade(const string& info, const TradeData& trade)
    {
        try
        {
            BaseLog::record_input_info(info);
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }

    void record_input_kline(const string& info, const KlineData& kline)
    {
        try
        {
            std::lock_guard<std::mutex> lk(input_statistic_map_mutex_);

            if (input_statistic_map_.find(info) == input_statistic_map_.end())
            {
                input_statistic_map_[info] = 1;
            }
            else
            {
                input_statistic_map_[info] += 1;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }

    void record_output_depth(const string& info, const SDepthQuote& quote)
    {
        try
        {
            BaseLog::record_output_info(info);
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }

    void record_output_trade(const string& info, const TradeData& trade)
    {
        try
        {
            BaseLog::record_output_info(info);
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }

    void record_output_kline(const string& info, const KlineData& kline)
    {
        try
        {
            std::lock_guard<std::mutex> lk(output_statistic_map_mutex_);

            if (output_statistic_map_.find(info) == output_statistic_map_.end())
            {
                output_statistic_map_[info] = 1;
            }
            else
            {
                output_statistic_map_[info] += 1;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
        }    
    }
};


#define COMM_LOG utrade::pandora::ThreadSafeSingleton<CommLog>::DoubleCheckInstance()

#define COMM_LOG_TRACE(info) COMM_LOG->log_trace_(LOG_HEADER + info)
#define COMM_LOG_DEBUG(info) COMM_LOG->log_debug_(LOG_HEADER + info)
#define COMM_LOG_INFO(info) COMM_LOG->log_info_(LOG_HEADER + info)
#define COMM_LOG_WARN(info) COMM_LOG->log_warn_(LOG_HEADER + info)
#define COMM_LOG_ERROR(info) COMM_LOG->log_error_(LOG_HEADER + info)
#define COMM_LOG_FATAL(info) COMM_LOG->log_fatal_(LOG_HEADER + info)

#define COMM_LOG_INPUT_DEPTH(info, depth) COMM_LOG->record_input_depth(info, depth)
#define COMM_LOG_INPUT_KLINE(info, depth) COMM_LOG->record_input_kline(info, depth)
#define COMM_LOG_INPUT_TRADE(info, depth) COMM_LOG->record_input_trade(info, depth)
#define COMM_LOG_OUTPUT_DEPTH(info, depth) COMM_LOG->record_output_depth(info, depth)
#define COMM_LOG_OUTPUT_KLINE(info, depth) COMM_LOG->record_output_kline(info, depth)
#define COMM_LOG_OUTPUT_TRADE(info, depth) COMM_LOG->record_output_trade(info, depth)

