#include "config.h"
#include <fstream>
#include <iostream>
#include "pandora/util/json.hpp"
#include "../log/log.h"

void Config::load_config(string file_name)
{
    try
    {    
        file_name_ = file_name;

        std::ifstream in_config(file_name);

        LOG_INFO("Config::load_config " + file_name );

        if (!in_config.is_open())
        {
            LOG_WARN("Failed to Open: " + file_name);
        }
        else
        {
            string contents((istreambuf_iterator<char>(in_config)), istreambuf_iterator<char>());

            // s_s << contents << "\n";

            LOG_INFO(contents);

            nlohmann::json js = nlohmann::json::parse(contents);
            
            if (!js["hub"].is_null() && !js["hub"]["risk_controller_addr"].is_null())
            {
                hub_address_ = js["hub"]["risk_controller_addr"].get<string>();
            }
            else
            {
                string error_msg = file_name + " does not have hub.addr! \n";
                // s_s << error_msg << "\n"; 
               LOG_ERROR(error_msg);
            }

            if (!js["ws_server"].is_null() && !js["ws_server"]["port"].is_null())
            {
                ws_port_ = js["ws_server"]["port"].get<int>();                
            }
            else
            {
                string error_msg = file_name + " does not have ws_server.port! \n";
                LOG_ERROR(error_msg);
            }

            if (!js["rest_server"].is_null() && !js["rest_server"]["port"].is_null())
            {
                rest_port_ = js["rest_server"]["port"].get<int>();
            }
            else
            {
                string error_msg = file_name + " does not have rest_server.port! \n";
                LOG_ERROR(error_msg);
            }   

            if (!js["market_cache"].is_null())
            {
                if (js["market_cache"]["frequency_list"].is_array())
                {
                    nlohmann::json frequency_list = js["market_cache"]["frequency_list"];
                    for (json::iterator it = frequency_list.begin(); it != frequency_list.end(); ++it)
                    {
                        json &value = *it;
                        frequency_list_.emplace(value.get<int>());                        
                    }
                }

                if (js["market_cache"]["frequency_numb"].is_number())
                {
                    frequency_numb_ = js["market_cache"]["frequency_numb"].get<int>();
                }

                if (js["market_cache"]["frequency_base"].is_array())
                {
                    nlohmann::json frequency_base_list = js["market_cache"]["frequency_base"];

                    for (json::iterator it = frequency_base_list.begin(); it != frequency_base_list.end(); ++it)
                    {
                        json &value = *it;
                        frequency_base_list_.emplace(value.get<int>());                        
                    }
                }                
            }              

            if (!js["is_dev_mode"].is_null())
            {
                is_dev_mode_ = js["is_dev_mode"].get<bool>();
            }
          
            if (!js["heartbeat_seconds"].is_null())
            {

                heartbeat_seconds =  js["heartbeat_seconds"].get<int>() > 2 ? js["heartbeat_seconds"].get<int>():3;  
            }

            // std::cout << "\nConfig: \n" + str() << std::endl;
            LOG_INFO("\nConfig: \n" + str());
            
        }
    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string Config::str()
{
try
    {
        std::stringstream s_s;
        s_s << "is_dev_mode: " << is_dev_mode_ << "\n"; 
        s_s << "frequency_list_: " << "\n";
        for (auto freq:frequency_list_)
        {
            s_s << freq << "\n";
        }       
        s_s << "frequency_numb_: " << frequency_numb_ << "\n";
        s_s << "frequency_base: " << "\n";
        for (auto freq:frequency_base_list_)
        {
            s_s << freq << "\n";
        }  
        s_s << "heartbeat_seconds: " << heartbeat_seconds <<"\n";
        return s_s.str();
    }
    catch(const std::exception& e)
    {
       LOG_ERROR(e.what());
    }

}