#include "config.h"
#include <fstream>
#include "pandora/util/json.hpp"
#include "../log/log.h"

void Config::load_config(string file_name)
{
    try
    {    
        std::ifstream in_config(file_name);

        if (!in_config.is_open())
        {
            cout << "Failed to Open: " << file_name << endl;
        }
        else
        {
            string contents((istreambuf_iterator<char>(in_config)), istreambuf_iterator<char>());
            nlohmann::json js = nlohmann::json::parse(contents);
            
            if (!js["hub"].is_null() && !js["hub"]["risk_controller_addr"].is_null())
            {
                hub_address_ = js["hub"]["risk_controller_addr"].get<string>();
            }
            else
            {
                string error_msg = file_name + " does not have hub.addr! \n";
                // cout << error_msg << endl; 
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
                        frequency_list_.push_back(value.get<int>());                        
                    }
                }

                if (js["market_cache"]["frequency_numb"].is_number())
                {
                    frequency_numb_ = js["market_cache"]["frequency_numb"].get<int>();
                }

                if (js["market_cache"]["frequency_base"].is_number())
                {
                    frequency_base_ = js["market_cache"]["frequency_base"].get<int>();
                }                
            }              

            cout << "frequency_list_: " << endl;
            for (auto freq:frequency_list_)
            {
                cout << freq << endl;
            }       
            cout << "frequency_numb_: " << frequency_numb_ << endl;
            cout << "frequency_base: " << frequency_base_ <<endl;
        }
    
    }
    catch(const std::exception& e)
    {
        std::cerr << "Config::load_config: " << e.what() << '\n';
    }
}