#include "test.h"
#include "../Log/log.h"
#include "../grpc_comm/server.h"

void test_grpc_server()
{
    GrpcServer server("0.0.0.0:5008");

    server.start();    
}

void TestMain()
{
    LOG_INFO("TestMain");
    
    test_grpc_server();
}