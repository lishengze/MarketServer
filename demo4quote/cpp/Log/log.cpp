#include "QuoteLog.h"
#include "pandora/util/time_util.h"
#include "../redis_quote_define.h"

QuoteLog::QuoteLog()
{
    
}



void QuoteLog::record_input_info(const string& info, const SDepthQuote& quote)
{
    try
    {
        record_input_info(info);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void QuoteLog::record_input_info(const string& info, const Trade& trade)
{
    try
    {
        record_input_info(info);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void QuoteLog::record_input_info(const string& info, const vector<KlineData>& klines)
{
    try
    {
        std::lock_guard<std::mutex> lk(input_statistic_map_mutex_);

        if (input_statistic_map_.find(info) == input_statistic_map_.end())
        {
            input_statistic_map_[info] = klines.size();
        }
        else
        {
            input_statistic_map_[info] += klines.size();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void QuoteLog::record_output_info(const string& info, const SDepthQuote& quote)
{
    try
    {
        record_output_info(info);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void QuoteLog::record_output_info(const string& info, const Trade& trade)
{
    try
    {
        record_output_info(info);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void QuoteLog::record_output_info(const string& info, const vector<KlineData>& klines)
{
    try
    {
        std::lock_guard<std::mutex> lk(output_statistic_map_mutex_);

        if (output_statistic_map_.find(info) == output_statistic_map_.end())
        {
            output_statistic_map_[info] = klines.size();
        }
        else
        {
            output_statistic_map_[info] += klines.size();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}
