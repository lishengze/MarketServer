#include "hub_config.h"

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
        grpc_server_addr_ = js["hub"]["addr"].get<string>();
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
