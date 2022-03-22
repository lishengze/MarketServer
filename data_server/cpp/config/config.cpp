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
            LOG_INFO(contents);

            nlohmann::json js = nlohmann::json::parse(contents);

            if (!js["database"].is_null())
            {
                database_info_.host_ = js["database"]["host"].get<std::string>();
                database_info_.port_ = js["database"]["port"].get<int>();
                database_info_.usr_ = js["database"]["usr"].get<std::string>();
                database_info_.pwd_ = js["database"]["pwd"].get<std::string>();
                database_info_.schema_ = js["database"]["schema"].get<std::string>();
            }
            else
            {
                LOG_ERROR("Config Need Database Info");
            }

            if (!js["kafka_ip"].is_null())
            {
                kafka_ip_ = js["kafka_ip_"].get<std::string>();
            }
            else
            {
                LOG_ERROR("Config Need Kafka Ip");
            }
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
        s_s << "database_info_: " << database_info_.str() << "\n"
            << "kafka_ip_: " << kafka_ip_ << "\n"; 
        return s_s.str();
    }
    catch(const std::exception& e)
    {
       LOG_ERROR(e.what());
    }

}