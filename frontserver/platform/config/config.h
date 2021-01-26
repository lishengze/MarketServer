#pragma once

#include "../front_server_declare.h"
#include "pandora/util/thread_safe_singleton.hpp"
#define CONFIG utrade::pandora::ThreadSafeSingleton<Config>::DoubleCheckInstance("./config.json")

class Config
{
    public:
        Config(string file_name) {
            load_config(file_name);                      
        }

        void set_file(string file_name)
        {
            load_config(file_name);
        }

        void load_config(string file_name);

        int get_ws_port() { return ws_port_;}

        int get_rest_port() { return rest_port_;}

        int get_frequency_numb() { return frequency_numb_;}
        std::vector<int>& get_frequency_base() { return frequency_base_list_;}
        std::vector<int>& get_frequency_list() { return frequency_list_;}

    private:
        string                  hub_address_;
        int                     ws_port_;   
        int                     rest_port_; 
        std::vector<int>        frequency_list_;
        int                     frequency_numb_{100};
        std::vector<int>        frequency_base_list_;

};