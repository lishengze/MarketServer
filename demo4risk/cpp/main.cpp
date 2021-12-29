#include "risk_controller.h"
#include "pandora/util/time_util.h"
#include "Log/log.h"
#include "global_declare.h"
// #include "util/tool.h"

#include "risk_controller_config.h"
#include "pandora/util/path_util.h"

// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, RiskControllerServer::signal_handler);
    signal(SIGINT, RiskControllerServer::signal_handler);
    signal(SIGHUP, RiskControllerServer::signal_handler);
    signal(SIGQUIT, RiskControllerServer::signal_handler);
    signal(SIGKILL, RiskControllerServer::signal_handler);
}

void test_log_print()
{
    #include <chrono>
    #include <thread>
    while(true)
    {
        _log_and_print("%s Test LOG AND PRINT! \n", utrade::pandora::NanoTimeStr().c_str());        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main(int argc, char** argv) {

    // setup the signal here
    setup_signal_handler_callback();

    string config_file_name = "config.json";
    if(argc == 2)
    {
        config_file_name = argv[1];
        cout << "config_file_name: " << config_file_name << endl;
    }

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

    CONFIG->parse_config(config_file_name);

    RiskControllerServer riskControllerServer(config_file_name);
    riskControllerServer.start();
    
    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}