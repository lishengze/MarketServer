#include "stream_engine.h"

int main(int argc, char** argv) {

    StreamEngine streamEngine;
    streamEngine.start();
    
    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}