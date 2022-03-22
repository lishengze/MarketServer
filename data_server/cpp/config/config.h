#pragma once

#include "../global_declare.h"
#include "pandora/util/thread_safe_singleton.hpp"
#define CONFIG utrade::pandora::ThreadSafeSingleton<Config>::DoubleCheckInstance()

#include "../data_struct/data_struct.h"

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

        string str();


    public:
        string                  file_name_;

        DBConnectInfo           database_info_;
        string                  kafka_ip_;
        string                  grpc_listen_ip_;
};

#define TEST_SYMBOL CONFIG->get_test_symbol()
#define DATABASE_INFO CONFIG->database_info_
#define KAFKA_IP CONFIG->kafka_ip_
#define GRPC_LISTEN_IP CONFIG->grpc_listen_ip_