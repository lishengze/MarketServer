#pragma once

#include "../global_declare.h"
#include "pandora/util/thread_safe_singleton.hpp"
#define CONFIG utrade::pandora::ThreadSafeSingleton<Config>::DoubleCheckInstance()

#include "../data_struct/data_struct.h"
#include "comm_type_def.h"

// using bcts::comm::MetaType;

USING_COMM_NAMESPACE

struct DataBaseInfo
{
    string Url;
    int Port;
    string UserName;
    string Password;
    string Schemas;

    string str()
    {
        std::stringstream s_s;
        s_s << "Url: " << Url << "\n"
            << "Port: " << Port << "\n"
            << "UserName: " << UserName << "\n"
            << "Password: " << Password << "\n"
            << "Schemas: " << Schemas << "\n";
        return s_s.str();
    }  
};

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

        void parse_meta_data(nlohmann::json& js);

        void parse_meta_data_atom(nlohmann::json& js, MetaType& meta);

        string str();

        string meta_str(MetaType& meta_data);


    public:
        string                  file_name_;

        DBConnectInfo           database_info_;
        string                  kafka_ip_;
        string                  grpc_listen_ip_;

        MetaType                kline_meta_data_;
        MetaType                trade_meta_data_;
        MetaType                depth_meta_data_;
};

#define TEST_SYMBOL CONFIG->get_test_symbol()
#define DATABASE_INFO CONFIG->database_info_
#define KAFKA_IP CONFIG->kafka_ip_
#define GRPC_LISTEN_IP CONFIG->grpc_listen_ip_

#define KLINE_META CONFIG->kline_meta_data_
#define TRADE_META CONFIG->trade_meta_data_
#define DEPTH_META CONFIG->depth_meta_data_