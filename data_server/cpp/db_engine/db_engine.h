#pragma once

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

#include "sql_scripts.h"

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "../data_struct/data_struct.h"


class DBEngine
{
    public:
        DBEngine(const DBConnectInfo& db_connect_info):db_connect_info_{db_connect_info}
        {
            ++DB_ID;
            db_id_ = DB_ID;
        }        

        bool connect();

        bool create_db(string db_name);

        bool check_db(string db_name);

        bool clean_db();

        bool prepare_statement();

        bool create_kline_table(const string& exchange, const string& symbol);

        bool insert_kline_data(const KlineData& kline_data);

        bool get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list);

        bool get_curr_table_list(std::set<string>& table_set);

        string str()
        {
            return "db_id: " +std::to_string(db_id_);
        }

    private:
        DBConnectInfo                   db_connect_info_;
        sql::Connection*                conn_{nullptr};

        sql::PreparedStatement*         stmt_create_kline_table_;
        sql::PreparedStatement*         stmt_insert_kline_data_;
        sql::PreparedStatement*         stmt_get_kline_data_;

        static int                      DB_ID;
        int                             db_id_;
};

using DBEnginePtr = boost::shared_ptr<DBEngine>;

class DBEnginePool
{
    public:
        DBEnginePool(const DBConnectInfo& db_connect_info):
        db_connect_info_{db_connect_info}
        {
            init_pool(db_connect_info);
        }

        bool init_pool(const DBConnectInfo& db_connect_info);

        DBEnginePtr get_db();

        void push_into_idle_list(DBEnginePtr db);

        void erase_from_work_list(DBEnginePtr db);

        bool check_idle_list();

        bool release_db(DBEnginePtr db);

        bool check_kline_table(const string& exchange, const string& symbol);

        bool update_table_list();

        bool create_kline_table(const string& exchange, const string& symbol);

        bool insert_kline_data(const KlineData& kline_data);

        bool get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list);

    private:
        DBConnectInfo               db_connect_info_;
        int                         max_engine_count{20};
        int                         init_engine_count{10};
        int                         cur_engine_count{0};

        std::set<DBEnginePtr>       idle_engine_list_;
        std::set<DBEnginePtr>       work_engine_list_;
        std::mutex                  engine_list_mutex_;

        std::set<string>            table_set_;
};

class TestEngine
{
public:
    TestEngine()
    {
        connect_info.host_ = "127.0.0.1";
        connect_info.port_ = 3306;
        connect_info.schema_ = "market";
        connect_info.usr_ = "bcts";
        connect_info.pwd_ = "bcts";

        test_create_table();
    }

    void start()
    {
        test_create_table();
    }

    void test_create_table()
    {
        LOG_INFO(connect_info.str());
        
        DBEnginePool  engine_pool{connect_info};
        // engine_pool.create_kline_table("FTX", "BTC_USDT");
    }
    DBConnectInfo   connect_info;
};