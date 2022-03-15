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
            LOG_ERROR(str() + " connect "+ db_connect_info_.str() +" failed!")
        }
        else
        {
            LOG_INFO(str() + "Connect " + db_connect_info_.str() +" successfully!")
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

bool create_db(string db_name);

bool check_db(string db_name);

bool clean_db();

bool prepare_statement();

bool create_kline_table(const string& exchange, const string& symbol);

bool insert_kline_data(const KlineData& kline_data);

bool get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list);


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
            idle_engine_list_.push_back(db);

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
        if (idle_engine_list_.empty())
        {
            if (cur_engine_count < max_engine_count)
            {
                LOG_INFO("Add IDLE DB CONN");
                for (int i=0; i<init_engine_count; ++i)
                {
                    ++cur_engine_count;
                    DBEnginePtr db_engine = boost::make_shared<DBEngine>(db_connect_info);
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

bool DBEnginePool::create_kline_table(const string& exchange, const string& symbol);

bool DBEnginePool::insert_kline_data(const KlineData& kline_data);

bool DBEnginePool::get_kline_data_list(const ReqKlineData& req_kline_info, std::list<KlineData> dst_list);
