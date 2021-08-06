#include "log.h"
#include "../config/config.h"
#include "../util/tools.h"
#include <stdio.h>

Log::Log()
{
    init_logger();
}

Log::~Log()
{
    if (statistic_thread_ && statistic_thread_->joinable())
    {
        statistic_thread_->join();
    }
}

void Log::start()
{
    try
    {
        statistic_thread_ = boost::make_shared<std::thread>(&Log::statistic_thread_main, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Log::record_input_info(const string& channel)
{
    try
    {
        std::lock_guard<std::mutex> lk(input_statistic_map_mutex_);

        if (input_statistic_map_.find(channel) == input_statistic_map_.end())
        {
            input_statistic_map_[channel] = 1;
        }
        else
        {
            input_statistic_map_[channel]++;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Log::record_input_info(const string& channel, const SDepthData& depth)
{
    try
    {
        record_input_info(channel);

        std::stringstream stream_obj;
        stream_obj  << "[Depth] " << depth.exchange << " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << "\n";
        log_source_input(stream_obj.str());

        if (CONFIG->get_dev_mode())
        {
            stream_obj << "--------- Ask ---------\n";
            for ( int i = 0; i < depth.ask_length; ++i)
            {
                stream_obj << depth.asks[i].price.get_value() << ", " << depth.asks[i].volume.get_value() << "\n";
            }
            stream_obj << "--------- Bid ---------\n";
            for ( int i = 0; i < depth.bid_length; ++i)
            {
                stream_obj << depth.bids[i].price.get_value() << ", " << depth.bids[i].volume.get_value() << "\n";
            }
            
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Log::record_input_info(const string& channel, const Trade& trade)
{
    try
    {
        record_input_info(channel);

        std::stringstream stream_obj;
        stream_obj << "[Trade] " << trade.exchange << " " << trade.symbol << " " << trade.price.get_value() << " " << trade.volume.get_value() << " \n";
        log_source_input(stream_obj.str());

        // if (CONFIG->get_dev_mode())
        // {
        //     log_trace(stream_obj.str());
        // }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Log::record_input_info(const string& channel, const vector<KlineData>& klines)
{
    try
    {
        {
            std::lock_guard<std::mutex> lk(input_statistic_map_mutex_);

            if (input_statistic_map_.find(channel) == input_statistic_map_.end())
            {
                input_statistic_map_[channel] = klines.size();
            }
            else
            {
                input_statistic_map_[channel] += klines.size();
            }
        }

        std::stringstream stream_obj;
        stream_obj << channel << "\n";
        log_source_input(stream_obj.str());

        if (CONFIG->get_dev_mode())
        {
            for(const KlineData& kline:klines)
            {
                
                stream_obj  << "[K-Kine] SRC " << get_sec_time_str(kline.index)  << ", " 
                            << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                            << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << "\n";
            }
            log_source_input(stream_obj.str());
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}


void Log::record_output_info(const string& info)
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
            output_statistic_map_[info]++;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::record_output_info(const string& channel, const string& details)
{
    try
    {
        record_output_info(channel);

        log_client_response(channel + "_" + details);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::record_output_info(const string& info, const SDepthData& depth)
{
    try
    {
        record_output_info(info);

        std::stringstream stream_obj;
        stream_obj  << "[O_Depth] " << info << "\n" << depth.exchange << " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << "\n";
        log_client_response(stream_obj.str()); 

        if (CONFIG->get_dev_mode())
        {
            stream_obj << "--------- Ask ---------\n";
            for ( int i = 0; i < depth.ask_length; ++i)
            {
                stream_obj << depth.asks[i].price.get_value() << ", " << depth.asks[i].volume.get_value() << "\n";
            }
            stream_obj << "--------- Bid ---------\n";
            for ( int i = 0; i < depth.bid_length; ++i)
            {
                stream_obj << depth.bids[i].price.get_value() << ", " << depth.bids[i].volume.get_value() << "\n";
            }
            log_client_response(stream_obj.str());   
        }           
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::record_output_info(const string& info, const Trade& trade)
{
    try
    {
        record_output_info(info);

        std::stringstream stream_obj;
        stream_obj << "[O_Trade] " << info << "\n" << trade.exchange << " " << trade.symbol << " " << trade.price.get_value() << " " << trade.volume.get_value() << " \n";
                
        log_client_response(stream_obj.str());

        // if (CONFIG->get_dev_mode())
        // {
        //     log_trace(stream_obj.str());
        // }                        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::record_output_info(const string& info, const vector<KlineData>& klines)
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
        std::cerr << e.what() << '\n';
    }    
}

void Log::record_output_info(const string& info, const std::vector<KlineDataPtr>& klines)
{
    try
    {
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

        std::stringstream stream_obj;
        stream_obj << info << "\n";
        log_client_response(stream_obj.str());

        if (CONFIG->get_dev_mode())
        {
            for(const KlineDataPtr& kline:klines)
            {
                
                stream_obj  << "[O_Kine] SRC " << get_sec_time_str(kline->index)  << ", " 
                            << "open: " << kline->px_open.get_value() << ", high: " << kline->px_high.get_value() << ", "
                            << "low: " << kline->px_low.get_value() << ", close: " << kline->px_close.get_value() << "\n";
            }
            log_client_response(stream_obj.str());
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

}

void Log::record_client_info(const string& client_id, const string& info)
{
    try
    {
        {
            std::lock_guard<std::mutex> lk(client_info_map_mutex_);

            if (client_info_map_.find(client_id) != client_info_map_.end())
            {
                std::vector<std::string> new_info_list ={info};
                client_info_map_[client_id] = new_info_list;
            }
            else
            {
                client_info_map_[client_id].push_back(info);
            }
        }

        {
            string detai_info = client_id + ": " + info;            
            log_client_request(detai_info);
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}


void Log::statistic_thread_main()
{
    try
    {
        while(true)
        {
            last_statistic_time_str_ = utrade::pandora::SecTimeStr();

            std::this_thread::sleep_for(std::chrono::seconds(statistic_secs_));

            print_statistic_data();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::print_statistic_data()
{
    try
    {
        print_input_info();

        print_output_info();

        print_client_info();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::print_input_info()
{
    try
    {
        std::lock_guard<std::mutex> lk(input_statistic_map_mutex_);
        std::stringstream stream_obj;

        stream_obj << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " INPUT: " << "\n";
        for (auto& iter:input_statistic_map_)
        {
            stream_obj << iter.first << ": " << iter.second << "\n";

            iter.second = 0;
        }
        stream_obj << "\n";
        
        log_info(stream_obj.str());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::print_output_info()
{
    try
    {
        std::lock_guard<std::mutex> lk(output_statistic_map_mutex_);

        std::stringstream stream_obj;

        stream_obj << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " OUTPUT: \n";
        std::list<std::string> clean_list;
        for (auto& iter:output_statistic_map_)
        {

            cout << iter.first << ": " << iter.second << "\n";

            if (iter.second == 0)            
            {
                clean_list.push_back(iter.first);
            }

            iter.second = 0;
        }

        for(string key:clean_list)
        {
            output_statistic_map_.erase(key);
        }

        stream_obj << "\n";
        log_info(stream_obj.str());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::print_client_info()
{
    try
    {
        std::lock_guard<std::mutex> lk(client_info_map_mutex_);

        std::stringstream stream_obj;
        stream_obj << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " Client: \n";
        for (auto iter1:client_info_map_)
        {
            stream_obj << "client_id: " << iter1.first << "\n";
            for (auto info:iter1.second)
            {
                stream_obj << "\t" << info << "\n";
            }
        }

        stream_obj << "\n";
        log_info(stream_obj.str());        
        client_info_map_.clear();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Log::init_logger()
{
    try
    {
        log4cplus::tstring file_pattern = LOG4CPLUS_TEXT("%D{%Y.%m.%d %H:%M:%S} [%F-%L] %-5p - %m %n");

        common_logger_ = boost::make_shared<log4cplus::Logger>(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT ("common")));
        common_logger_->setLogLevel(log4cplus::TRACE_LOG_LEVEL);

        trace_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/trace_log.log"), 100*1024*1024, 2, true, false);
        trace_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        trace_appender_->setThreshold(log4cplus::TRACE_LOG_LEVEL);

        info_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/info_log.log"), 100*1024*1024, 2, true, true);
        info_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        info_appender_->setThreshold(log4cplus::INFO_LOG_LEVEL);

        warn_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/warn_log.log"), 100*1024*1024, 2, true, true);
        warn_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        warn_appender_->setThreshold(log4cplus::WARN_LOG_LEVEL);          

        common_logger_->addAppender(trace_appender_);
        common_logger_->addAppender(info_appender_);     
        common_logger_->addAppender(warn_appender_);      

        debug_logger_ = boost::make_shared<log4cplus::Logger>(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT ("debug")));
        debug_logger_->setLogLevel(log4cplus::DEBUG_LOG_LEVEL);

        debug_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/debug_log.log"), 100*1024*1024, 2, true, true);
        debug_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        debug_appender_->setThreshold(log4cplus::DEBUG_LOG_LEVEL);
        debug_logger_->addAppender(debug_appender_);

        client_request_logger_ = boost::make_shared<log4cplus::Logger>(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT ("client_request")));
        client_request_logger_->setLogLevel(log4cplus::TRACE_LOG_LEVEL);

        client_request_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/client_request.log"), 100*1024*1024, 2, true, true);
        client_request_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        client_request_appender_->setThreshold(log4cplus::INFO_LOG_LEVEL);      
        client_request_logger_->addAppender(client_request_appender_);          

        client_response_logger_ = boost::make_shared<log4cplus::Logger>(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT ("client_response")));
        client_response_logger_->setLogLevel(log4cplus::TRACE_LOG_LEVEL);

        client_response_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/client_response.log"), 100*1024*1024, 2, true, true);
        client_response_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        client_response_appender_->setThreshold(log4cplus::INFO_LOG_LEVEL);       
        client_response_logger_->addAppender(client_response_appender_);            

        source_input_logger_ = boost::make_shared<log4cplus::Logger>(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT ("source_input")));
        source_input_logger_->setLogLevel(log4cplus::TRACE_LOG_LEVEL);

        source_input_appender_ = new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT("log/source_input.log"), 100*1024*1024, 2, true, true);
        source_input_appender_->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(file_pattern)));
        source_input_appender_->setThreshold(log4cplus::INFO_LOG_LEVEL);             
        source_input_logger_->addAppender(source_input_appender_);   
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Log::log_trace(const string& info)
{
    try
    {
        LOG4CPLUS_TRACE(*common_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_debug(const string& info)
{
    try
    {
        if (CONFIG->get_dev_mode())
        {
            LOG4CPLUS_DEBUG(*common_logger_.get(), info);
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_info(const string& info)
{
    try
    {
        LOG4CPLUS_INFO(*common_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_warn(const string& info)
{
    try
    {
        LOG4CPLUS_WARN(*common_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_error(const string& info)
{
    try
    {
        printf(info.c_str());
        LOG4CPLUS_ERROR(*common_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_fatal(const string& info)
{
    try
    {
        printf(info.c_str());
        LOG4CPLUS_FATAL(*common_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_client_request(const string& info)
{
    try
    {
        LOG4CPLUS_INFO(*client_request_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_client_response(const string& info)
{
    try
    {
        LOG4CPLUS_INFO(*client_response_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_source_input(const string& info)
{
    try
    {
        LOG4CPLUS_INFO(*source_input_logger_.get(), info);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void Log::log_exception(const string& info)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

