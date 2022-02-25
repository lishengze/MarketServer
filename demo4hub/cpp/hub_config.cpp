#include "hub_config.h"

#include <iostream>
using std::cout;
using std::endl;

void Config::parse_config(const std::string& file_name)
{
    // init logger
    

    try
    {
        

        logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

        // read the config file
        std::ifstream in_config(file_name);
        std::string contents((std::istreambuf_iterator<char>(in_config)), std::istreambuf_iterator<char>());
        // std::cout << contents << std::endl;
        njson js = njson::parse(contents);

        LOG_INFO("hub_config: Config File Name : " + file_name + ", \ncontent: " + contents);

        // grpc
        risk_controller_addr_ = js["hub"]["risk_controller_addr"].get<string>();
        stream_engine_addr_ = js["hub"]["stream_engine_addr"].get<string>();

        LOG_INFO("risk_controller_addr_: " + risk_controller_addr_ + ", stream_engine_addr_: " + stream_engine_addr_);
    }
    catch (std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}
