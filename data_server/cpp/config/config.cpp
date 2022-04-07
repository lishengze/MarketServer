#include "config.h"
#include <fstream>
#include <iostream>
#include "pandora/util/json.hpp"
#include "../Log/log.h"

USING_COMM_NAMESPACE

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
                kafka_ip_ = js["kafka_ip"].get<std::string>();
            }
            else
            {
                LOG_ERROR("Config Need Kafka Ip");
            }


            if (!js["grpc_listen_ip"].is_null())
            {
                grpc_listen_ip_ = js["grpc_listen_ip"].get<std::string>();
            }
            else
            {
                LOG_ERROR("Config Need grpc_listen_ip");
            }

            LOG_INFO("\nConfig: \n" + str());            
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void Config::parse_meta_data_atom(nlohmann::json& js, MetaType& meta)
{
    try
    {
        meta.clear();
        for (auto iter = js.begin() ; iter != js.end() ; ++iter)
        {
            string exchange = iter.key();
            nlohmann::json& symbol_list = iter.value();

            for (auto symbol_iter = js.begin(); symbol_iter != js.end() ; ++symbol_iter)
            {
                string symbol = (*iter).get<string>();
                meta[symbol].emplace(exchange);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}


void Config::parse_meta_data(nlohmann::json& js)
{
    try
    {
        if (!js["meta_data"].is_null())
        {
            nlohmann::json& meta_data_js = js["meta_data"];

            if (!meta_data_js["kline"].is_null())
            {
                nlohmann::json& src_data = meta_data_js["kline"];

                parse_meta_data_atom(src_data, kline_meta_data_);
            }

            if (!meta_data_js["Trade"].is_null())
            {
                nlohmann::json& src_data = meta_data_js["Trade"];

                parse_meta_data_atom(src_data, trade_meta_data_);
            }

            if (!meta_data_js["Depth"].is_null())
            {
                nlohmann::json& src_data = meta_data_js["Depth"];

                parse_meta_data_atom(src_data, depth_meta_data_);
            }                        
        }
        else
        {
            LOG_ERROR("Config Need meta_data");
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