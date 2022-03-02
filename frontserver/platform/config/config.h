#pragma once

#include "../front_server_declare.h"
#include "pandora/util/thread_safe_singleton.hpp"
#define CONFIG utrade::pandora::ThreadSafeSingleton<Config>::DoubleCheckInstance()

class Config
{
    public:
        Config(string file_name="") {
            if (file_name != "")
            {
                load_config(file_name); 
            }                                 
        }

        void set_file(string file_name)
        {
            load_config(file_name);
        }

        void load_config(string file_name);

        int get_ws_port() { return ws_port_;}

        int get_rest_port() { return rest_port_;}

        int get_frequency_numb() { return frequency_numb_;}

        int get_heartbeat_secs() {return heartbeat_seconds;}

        int get_statistic_secs() {return statistic_secs_;}

        string get_test_symbol() {return test_symbol;}

        bool get_dev_mode() { return is_dev_mode_;}

        std::set<int>& get_frequency_base() { return frequency_base_list_;}
        std::set<int>& get_frequency_list() { return frequency_list_;}
        string get_file_name() { return file_name_;}

        string str();

    private:
        string                  file_name_;
        string                  hub_address_;
        int                     ws_port_;   
        int                     rest_port_; 
        std::set<int>           frequency_list_;
        int                     frequency_numb_{100};
        std::set<int>           frequency_base_list_;

        bool                    is_dev_mode_{true};
        int                     heartbeat_seconds{5};
        int                     statistic_secs_{10};
        string                  test_symbol{""};

};

#define TEST_SYMBOL CONFIG->get_test_symbol()