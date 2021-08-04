#include "hub_config.h"

#include <iostream>
using std::cout;
using std::endl;

void Config::parse_config(const std::string& file_name)
{
    // init logger
    

    try
    {
        cout << "hub_config: Config File Name : " << file_name << endl;

        logger_ = utrade::pandora::UTLog::getStrategyLogger("StreamEngine", "StreamEngine");

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
        risk_controller_addr_ = js["hub"]["risk_controller_addr"].get<string>();
        stream_engine_addr_ = js["hub"]["stream_engine_addr"].get<string>();

        cout << "risk_controller_addr_: " << risk_controller_addr_ << endl;
        cout << "stream_engine_addr_: " << stream_engine_addr_ << endl;
        
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
