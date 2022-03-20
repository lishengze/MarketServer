#include "db_engine.h"
#include "../Log/log.h"

int DBEngine::DB_ID = 0;

bool DBEngine::connect()
{
    try
    {
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();

        sql::ConnectOptionsMap connection_properties;
        connection_properties["hostName"] = db_connect_info_.host_;
        connection_properties["userName"] = db_connect_info_.usr_;
        connection_properties["password"] = db_connect_info_.pwd_;
        connection_properties["schema"] = db_connect_info_.schema_;
        connection_properties["port"] = db_connect_info_.port_;
        connection_properties["OPT_RECONNECT"] = true;

        conn_ = driver->connect(connection_properties);
        
        if (conn_ == NULL)
        {
            LOG_ERROR(str() + ", connect "+ db_connect_info_.str() +" failed!");
            return false;
        }
        else
        {
            LOG_INFO(str() + ", Connect " + db_connect_info_.str() +" successfully!");
            return true;
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DBEngine::create_db(string db_name)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DBEngine::check_db(string db_name)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

bool DBEngine::clean_db()
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

bool DBEngine::prepare_statement()
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

bool DBEngine::create_kline_table(const string& exchange, const string& symbol)
{
    try
    {
        string kline_talbe_name = get_kline_table_name(exchange, symbol);

        if (table_set_.size() == 0)
        {
            update_table_list();
        }

        init_kline_prestmt(exchange, symbol);

        if (table_set_.find(kline_talbe_name) == table_set_.end())
        {
            string sql_str = get_create_kline_sql_str(exchange, symbol);
            sql::Statement* stmt = conn_->createStatement();
            stmt->execute(sql_str);

            update_table_list();

            LOG_INFO(table_info());

            if (table_set_.find(kline_talbe_name) == table_set_.end())
            {
                LOG_ERROR("Create " + kline_talbe_name + " Failed!");
                return false;
            }
            else
            {
                LOG_INFO("Create " + kline_talbe_name + " Successfully!");
                return true;
            }
        }
        else
        {

            LOG_WARN("Table " + kline_talbe_name + " was created!");
        }

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

void DBEngine::update_table_list()
{
    try
    {
        if (conn_)
        {
            string sql_str = get_all_tables();
            sql::Statement* stmt = conn_->createStatement();
            sql::ResultSet* result = stmt->executeQuery(sql_str);

            while(result->next())
            {
                string table = result->getString(1);

                // LOG_INFO("table: " + table);

                table_set_.emplace(table);
            }
        }
        else
        {
            LOG_ERROR("conn_ is null");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string DBEngine::table_info()
{
    try
    {
        string result = "";
        for (auto table:table_set_)
        {
            result += table + ",";
        }
        if (table_set_.empty()) result = "table is empty";
        result += "\n";
        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}


void DBEngine::init_kline_prestmt(const string& exchange, const string& symbol)
{
    try
    {
        string kline_table = get_kline_table_name(exchange, symbol);

        if (kline_stmt_map.find(kline_table) == kline_stmt_map.end())
        {
            kline_stmt_map.emplace(kline_table, PrepareSMT(conn_, exchange, symbol));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool DBEngine::insert_kline_data(const KlineData& kline_data)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

bool DBEngine::get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

bool DBEngine::get_curr_table_list(std::set<string>& table_set)
{
    try
    {
        if (table_set_.empty()) update_table_list();

        table_set = table_set_;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DBEnginePool::init_pool(const DBConnectInfo& db_connect_info)
{
    try
    {
        for (int i=0; i<init_engine_count; ++i)
        {
            ++cur_engine_count;
            DBEnginePtr db_engine = boost::make_shared<DBEngine>(db_connect_info);
            if (db_engine->connect())
            {
                push_into_idle_list(db_engine);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DBEnginePool::push_into_idle_list(DBEnginePtr db)
{
    try
    {
        std::lock_guard<std::mutex> lk(engine_list_mutex_);

        if (idle_engine_list_.find(db) == idle_engine_list_.end())
        {
            idle_engine_list_.insert(db);

            LOG_TRACE("Idle list Push DB " + db->str());
        }
        else
        {
            LOG_TRACE(db->str() + " already in idle list");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DBEnginePool::erase_from_work_list(DBEnginePtr db)
{
    try
    {
        std::lock_guard<std::mutex> lk(engine_list_mutex_);

        if (work_engine_list_.find(db) == work_engine_list_.end())
        {
            LOG_TRACE(db->str() + " not in work_db_list;");
        }
        else
        {
            work_engine_list_.erase(db);
            LOG_TRACE("work_list erase: " + db->str());
        }

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool DBEnginePool::check_idle_list()
{
    try
    {
        std::lock_guard<std::mutex> lk(engine_list_mutex_);

        LOG_INFO("idle_engine_list_.size: " + std::to_string(idle_engine_list_.size()));

        if (idle_engine_list_.empty())
        {
            if (cur_engine_count < max_engine_count)
            {
                LOG_INFO("Add IDLE DB CONN");
                for (int i=0; i<init_engine_count; ++i)
                {
                    ++cur_engine_count;
                    DBEnginePtr db_engine = boost::make_shared<DBEngine>(db_connect_info_);
                    if (db_engine->connect())
                    {
                        idle_engine_list_.emplace(db_engine);
                    }
                }

                if (idle_engine_list_.size() > 0)
                {
                    return true;
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            return true;
        }

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

DBEnginePtr DBEnginePool::get_db()
{
    DBEnginePtr result = nullptr;
    try
    {
        int wait_counts = 10;
        while (!check_idle_list() && wait_counts--)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (check_idle_list())
        {
            std::lock_guard<std::mutex> lk(engine_list_mutex_);
            result = *(idle_engine_list_.begin());
            idle_engine_list_.erase(result);
            work_engine_list_.insert(result);
        }
        else
        {
            LOG_ERROR("All DB Con is Busy!");
        }

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return result;
}

bool DBEnginePool::release_db(DBEnginePtr db)
{
    try
    {
        erase_from_work_list(db);

        push_into_idle_list(db);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DBEnginePool::create_kline_table(const string& exchange, const string& symbol)
{
    try
    {
        DBEnginePtr db_engine = get_db();

        if (db_engine)
        {
            db_engine->create_kline_table(exchange, symbol);

            release_db(db_engine);
        }
        else
        {
            LOG_WARN("No DB Engine Available");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool DBEnginePool::insert_kline_data(const KlineData& kline_data)
{
    try
    {
        if (!check_kline_table(kline_data.exchange,kline_data.symbol))
        {
            create_kline_table(kline_data.exchange, kline_data.symbol);
        }

        DBEnginePtr db_engine = get_db();

        if (db_engine)
        {

            db_engine->insert_kline_data(kline_data);

            release_db(db_engine);
        }
        else
        {
            LOG_WARN("No DB Engine Available");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
    
}

bool DBEnginePool::check_kline_table(const string& exchange, const string& symbol)
{
    try
    {
        if (table_set_.size() == 0)
        {
            update_table_list();
        }

        string kline_table_name = get_kline_table_name(exchange, symbol);

        if (table_set_.find(kline_table_name) == table_set_.end())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return false;
}

bool DBEnginePool::update_table_list()
{
    try
    {
        DBEnginePtr db_engine = get_db();

        if (db_engine)
        {
            db_engine->get_curr_table_list(table_set_);
        }
        else
        {
            LOG_WARN("Update_table_list Failed! No DB Engine Available!");
        }        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return false;
}

bool DBEnginePool::get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list)
{
    try
    {
        DBEnginePtr db_engine = get_db();

        if (db_engine)
        {
            db_engine->get_kline_data_list(req_kline_info, dst_list);

            release_db(db_engine);
        }
        else
        {
            LOG_WARN("No DB Engine Available");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
    
}

void TestEngine::start()
{
    test_create_table();
}

void TestEngine::test_create_table()
{
    LOG_INFO(connect_info.str());

    DBEnginePool  engine_pool{connect_info};
    engine_pool.create_kline_table("FTX", "ETH_USDT");
}