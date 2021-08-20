#include "stream_engine_config.h"
#include "stream_engine.h"
#include "kline_mixer_test.h"
#include "pandora/util/io_service_pool.h"
#include "report.h"

#include "Log/log.h"
#include "global_declare.h"
#include "pandora/util/path_util.h"

// config file relative path
const char* config_file = "config.json";

// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, StreamEngine::signal_handler);
    signal(SIGINT, StreamEngine::signal_handler);
    signal(SIGHUP, StreamEngine::signal_handler);
    signal(SIGQUIT, StreamEngine::signal_handler);
    signal(SIGKILL, StreamEngine::signal_handler);
}

int main(int argc, char** argv) 
{
    // test_kline_calculator60();
    // test_kline_calculator3600();
    // test_kline_hubber();
    
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

    // init configuration
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file_name);

    StreamEngine streamEngine;
    streamEngine.init();
    streamEngine.start();

    // Report report;
    // report.start();
    
    utrade::pandora::io_service_pool engine_pool(1);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}