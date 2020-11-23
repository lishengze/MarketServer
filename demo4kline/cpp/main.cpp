#include "kline_server.h"


// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, KlineServer::signal_handler);
    signal(SIGINT, KlineServer::signal_handler);
    signal(SIGHUP, KlineServer::signal_handler);
    signal(SIGQUIT, KlineServer::signal_handler);
    signal(SIGKILL, KlineServer::signal_handler);
}

int main(int argc, char** argv) {

    // setup the signal here
    setup_signal_handler_callback();

    KlineServer klineServer;
    klineServer.start();
    
    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}