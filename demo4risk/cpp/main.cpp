#include "risk_controller.h"


// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, RiskControllerServer::signal_handler);
    signal(SIGINT, RiskControllerServer::signal_handler);
    signal(SIGHUP, RiskControllerServer::signal_handler);
    signal(SIGQUIT, RiskControllerServer::signal_handler);
    signal(SIGKILL, RiskControllerServer::signal_handler);
}

int main(int argc, char** argv) {

    // setup the signal here
    setup_signal_handler_callback();

    RiskControllerServer riskControllerServer;
    riskControllerServer.start();
    
    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}