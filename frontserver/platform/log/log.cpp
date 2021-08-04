#include "log.h"

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

void Log::record_input_info(const string& channel, const SDepthData& quote)
{
    try
    {
        record_input_info(channel);
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
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Log::record_output_info(const string& info, const SDepthData& quote)
{
    try
    {
        record_output_info(info);
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

void Log::record_client_info(const string& client_id, const string& info)
{
    try
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

        cout << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " INPUT: " << endl;
        for (auto& iter:input_statistic_map_)
        {
            cout << iter.first << ": " << iter.second << endl;

            iter.second = 0;
        }
        cout << endl;

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

        cout << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " OUTPUT: " << endl;
        std::list<std::string> clean_list;
        for (auto& iter:output_statistic_map_)
        {

            cout << iter.first << ": " << iter.second << endl;

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

        cout << endl;

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

        cout << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " Client: " << endl;
        for (auto iter1:client_info_map_)
        {
            cout << "client_id: " << iter1.first << endl;
            for (auto info:iter1.second)
            {
                cout << info << endl;
            }
        }

        client_info_map_.clear();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}