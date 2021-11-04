#include "log.h"
#include "../config/config.h"
#include "../util/tools.h"
#include <stdio.h>

FrontServerLog::FrontServerLog()
{
}

FrontServerLog::~FrontServerLog()
{
    if (statistic_thread_ && statistic_thread_->joinable())
    {
        statistic_thread_->join();
    }
}

void FrontServerLog::record_input_info(const string& channel, const SDepthData& depth)
{
    try
    {
        record_input_info(channel);

        std::stringstream stream_obj;
        stream_obj  << "[Depth] " << depth.exchange << " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length;
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
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }
    
}

void FrontServerLog::record_input_info(const string& channel, const Trade& trade)
{
    try
    {
        std::stringstream stream_obj;
        stream_obj << "[Trade] " << trade.exchange << " " << trade.symbol << " " 
                   << trade.price.get_value() << " " << trade.volume.get_value();

        log_source_input(stream_obj.str());  

        record_input_info(channel);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }
}

void FrontServerLog::record_input_info(const string& channel, const vector<KlineData>& klines)
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
        stream_obj << channel;
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
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }
}


void FrontServerLog::record_output_info(const string& info, const SDepthData& depth)
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
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void FrontServerLog::record_output_info(const string& info, const Trade& trade)
{
    try
    {
        record_output_info(info);

        std::stringstream stream_obj;
        stream_obj << "[O_Trade] " << info << "\n" << trade.exchange << " " << trade.symbol << " " 
                   << trade.price.get_value() << " " << trade.volume.get_value() << " \n";
                
        log_client_response(stream_obj.str());                   
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void FrontServerLog::record_output_info(const string& info, const vector<KlineData>& klines)
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

void FrontServerLog::record_output_info(const string& info, const std::vector<KlineDataPtr>& klines)
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
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }

}

void FrontServerLog::record_client_info(const string& client_id, const string& info)
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

        // {
        //     string detai_info = client_id + ": " + info;            
        //     log_client_request(detai_info);
        // }

    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}

void FrontServerLog::log_client_request(const string& info)
{
    try
    {
        LOG4CPLUS_INFO(*client_request_logger_.get(), info);        
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }    
}

void FrontServerLog::log_debug(const string& info)
{
    try
    {
        if (CONFIG->get_dev_mode())
        {

            LOG4CPLUS_DEBUG(*debug_logger_.get(), info);
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }    
}
