#include "server_engine.h"
#include "config/config.h"

#include "log/log.h"
#include "pandora/util/path_util.h"

void setup_signal_handler_callback()
{
    signal(SIGTERM, ServerEngine::signal_handler);
    signal(SIGINT, ServerEngine::signal_handler);
    signal(SIGHUP, ServerEngine::signal_handler);
    signal(SIGQUIT, ServerEngine::signal_handler);
    signal(SIGKILL, ServerEngine::signal_handler);
    signal(SIGABRT, ServerEngine::signal_handler);
    signal(SIGSEGV, ServerEngine::signal_handler);
}


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

int main(int argc, char** argv)
{
    try
    {
        setup_signal_handler_callback();
        
        init_log(argv);

        utrade::pandora::io_service_pool engine_pool(4);

        string config_file_name = "config.json";
        if(argc == 2)
        {
            config_file_name = argv[1];
            cout << "config_file_name: " << config_file_name << endl;
        }
        CONFIG->load_config(config_file_name);

        // LOG->set_statistic_secs_(CONFIG->get_heartbeat_secs());
    
        utrade::pandora::ThreadSafeSingleton<ServerEngine>::DoubleCheckInstance(engine_pool);

        // start pool
        engine_pool.start();

        SERVER_EENGINE->launch();

        // launch the engine
        engine_pool.block();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << " ";
    }    
}