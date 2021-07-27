#include "log.h"
#include "pandora/util/time_util.h"

Log::Log()
{
    
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

void Log::record_output_info(const string& info)
{
    try
    {
        /* code */
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
        cout << "From: " << last_statistic_time_str_ << " To: " << utrade::pandora::SecTimeStr() << " Input Msg: " << endl;
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
