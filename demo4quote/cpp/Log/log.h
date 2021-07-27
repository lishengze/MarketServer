#include "../global_declare.h"

#include "pandora/util/thread_safe_singleton.hpp"

class Log 
{
public:

    ~Log();

    Log();

    void start();

    void record_input_info(const string& channel);

    void record_output_info(const string& info);

    void statistic_thread_main();

    void print_statistic_data();

private:



    int                             statistic_secs_{10};
    boost::shared_ptr<std::thread>  statistic_thread_{nullptr};

    string                          last_statistic_time_str_;

    map<string, int>                input_statistic_map_;
    std::mutex                      input_statistic_map_mutex_;

    map<string, int>                output_statistic_map_;
    std::mutex                      output_statistic_map_mutex_;

};

#define LOG utrade::pandora::ThreadSafeSingleton<Log>::Instance()