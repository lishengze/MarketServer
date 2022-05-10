#include <iostream>
#include "pandora/util/io_service_pool.h"
#include "pandora/redis/redis_api.h"

#include "Log/log.h"
#include "config.h"

using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;

void init_log(char** argv)
{
    string program_full_name = argv[0];

    string work_dir = utrade::pandora::get_work_dir_name (program_full_name);
    string program_name = utrade::pandora::get_program_name(program_full_name);

    cout << "program_full_name: " << program_full_name << "\n"
            << "work_dir: " << work_dir << "\n"
            << "program_name: " << program_name << "\n"
            << endl;

    LOG->set_work_dir(work_dir);
    LOG->set_program_name(program_name);
    LOG->start();   
}

class RedisQuote : public utrade::pandora::CRedisSpi
{
    public:
    
    RedisQuote() { 
        ConnectRedis();
        StartTest();
    }

    ~RedisQuote() {
        if (test_thread_.joinable()) {
            test_thread_.join();
        }
    }

    void ConnectRedis() 
    {
        LOG_INFO("---- Connect Redis Start ----");
        LOG_INFO("Connect Redis 1");
        redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
        LOG_INFO("Connect Redis 2");
        redis_api_->RegisterSpi(this);
        LOG_INFO("Connect Redis 3");
        redis_api_->RegisterRedis(CONFIG->quote_redis_host_, CONFIG->quote_redis_port_, CONFIG->quote_redis_password_, utrade::pandora::RM_Subscribe);        
        LOG_INFO("Connect Redis 4");
    }

    void StartTest() { 
        test_thread_ = std::thread(&RedisQuote::TestMain, this);
    }

    void TestMain() 
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));

            ConnectRedis();
        }
    }

    void OnConnected() {
        LOG_INFO("OnConnected");
    };
    // redis disconnect notify
    void OnDisconnected(int status){

    }
    // redis message notify
    void OnMessage(const std::string& channel, const std::string& msg){

    }
    // redis get message notify
    void OnGetMessage(const std::string& key, const std::string& value){

    }    

    private:
        RedisApiPtr         redis_api_;

        std::thread         test_thread_;
};

void TestRedis() { 
    RedisQuote quote;
}


int main(int argc, char** argv) { 
    init_log(argv);
    string file_name = "config.json";

    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(file_name);

    TestRedis();

    return 1;
}