#include "server_engine.h"

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

int main()
{
    try
    {
        setup_signal_handler_callback();

        utrade::pandora::io_service_pool engine_pool(3);

        utrade::pandora::Singleton<ServerEngine>::Instance(engine_pool);

        SERVER_EENGINE->launch();

        // start pool
        engine_pool.start();

        // launch the engine
        engine_pool.block();

        // destroy config object
        DESTROY_SERVER_ENGINE;
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << " ";
    }    
}