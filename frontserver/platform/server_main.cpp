#include "server_engine.h"
#include "config/config.h"

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

int main(int argc, char** argv)
{
    try
    {
        setup_signal_handler_callback();

        utrade::pandora::io_service_pool engine_pool(4);

        string config_file_name = "config.json";
        if(argc == 2)
        {
            config_file_name = argv[1];
            cout << "config_file_name: " << config_file_name << endl;
        }
        CONFIG->load_config(config_file_name);

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