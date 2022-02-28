#include "stream_engine_config.h"
#include "stream_engine.h"
#include "Log/log.h"

void Config::parse_config(const std::string& file_name)
{
    // init logger
    logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

    try
    {
        std::cout << "\n***** Config FileName:  " << file_name << std::endl;

        LOG_INFO("\n***** Config FileName:  " + file_name);

        // read the config file
        std::ifstream in_config(file_name);
        std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());

        LOG_INFO(contents);
        
        njson js = njson::parse(contents);

        // grpc
        grpc_publish_addr_ = js["grpc"]["publish_addr"].get<string>();

        // system
        mode_ = js["debug"]["mode"].get<string>();
        sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
        dump_ = bool(js["debug"]["dump"].get<int>());
        replay_ratio_ = js["debug"]["replay_ratio"].get<int>();
        replay_replicas_ = js["debug"]["replay_replicas"].get<int>();

        // redis quote
        quote_redis_host_ = js["redis_quote"]["host"].get<string>();
        quote_redis_port_ = js["redis_quote"]["port"].get<int>();
        quote_redis_password_ = js["redis_quote"]["password"].get<string>();
        quote_redis_snap_interval_ = js["redis_quote"]["snap_interval"].get<int>();

        // reporter
        // reporter_addr_ = js["reporter"]["addr"].get<string>();
        // reporter_port_ = js["reporter"]["port"].get<string>();

        // nacos
        nacos_addr_ = js["nacos"]["addr"].get<string>();
        nacos_namespace_ = js["nacos"]["namespace"].get<string>();
        
        check_secs = js["check_secs"].get<int>();
        

    }
    catch (std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch (...)
    {
        std::cerr << "Config Unknown Exception! " << std::endl;
    }
}

string Config::get_config() const 
{
    std::unique_lock<std::mutex> inner_lock{ mutex_faw_config_ };
    return faw_config_;
}

void Config::set_config(const string& config)
{
    // _log_and_print("set config %s", config);
    std::unique_lock<std::mutex> inner_lock{ mutex_faw_config_ };
    faw_config_ = config;
}
