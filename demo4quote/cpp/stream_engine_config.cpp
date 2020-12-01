#include "stream_engine_config.h"
#include "stream_engine.h"

void Config::parse_config(const std::string& file_name)
{
    // init logger
    logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

    try
    {
        // get the running module path
        //string WorkFolder = utrade::pandora::get_module_path();
        // append the prefix
        //string intact_file_name = WorkFolder + file_name;
        UT_LOG_INFO(logger_, "System Parse Config File " << file_name);
        // read the config file
        std::ifstream in_config(file_name);
        std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
        // std::cout << contents << std::endl;
        njson js = njson::parse(contents);

        // grpc
        grpc_publish_addr_ = js["grpc"]["publish_addr"].get<string>();
        grpc_publish_depth_ = js["grpc"]["publish_depth"].get<int>();;
        grpc_publish_frequency_ = js["grpc"]["frequency"].get<int>();
        grpc_publish_raw_frequency_ = js["grpc"]["raw_frequency"].get<int>();

        // system
        sample_symbol_ = js["debug"]["sample_symbol"].get<string>();
        publish_data_ = bool(js["debug"]["publish_data"].get<int>());
        dump_binary_ = bool(js["debug"]["dump_binary"].get<int>());
        output_to_screen_ = bool(js["debug"]["output_to_screen"].get<int>());
        replay_mode_ = bool(js["debug"]["replay_mode"].get<int>());
        replay_ratio_ = js["debug"]["replay_ratio"].get<int>();

        // redis quote
        quote_redis_host_ = js["redis_quote"]["host"].get<string>();
        quote_redis_port_ = js["redis_quote"]["port"].get<int>();
        quote_redis_password_ = js["redis_quote"]["password"].get<string>();
        quote_redis_snap_interval_ = js["redis_quote"]["snap_interval"].get<int>();

        // config center
        nacos_addr_ = js["nacos"]["addr"].get<string>();
        for (auto iter = js["include"]["symbols"].begin(); iter != js["include"]["symbols"].end(); ++iter) {
            const string& symbol = *iter;
            include_symbols_.insert(symbol);
        }
        for (auto iter = js["include"]["exchanges"].begin(); iter != js["include"]["exchanges"].end(); ++iter) {
            const string& exchange = *iter;
            include_exchanges_.insert(exchange);
        }                
        symbol_precise_["BTC_USDT"] = 1;
        symbol_precise_["ETH_USDT"] = 2;
        symbol_precise_["ETH_BTC"] = 6;
        symbol_precise_["XLM_BTC"] = 6;
        symbol_precise_["XLM_ETH"] = 6;
        symbol_precise_["IRIS_USDT"] = 4;
        symbol_precise_["HT_USDT"] = 4;
        UT_LOG_INFO(logger_, "Parse Config finish.");
    }
    catch (std::exception& e)
    {
        std::cerr << "Config parse exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Config Unknown Exception! " << std::endl;
    }
}

void Config::set_configuration_precise(const std::unordered_map<string, int>& vals)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_configuration_ };
        for( const auto& v : vals ) {
            symbol_precise_[v.first] = v.second;
            _log_and_print("set precise: %s %d", v.first.c_str(), v.second);
        }
    }

    for( const auto& v : vals ) {
        STREAMENGINE->on_precise_changed(v.first, v.second);
    }
}
