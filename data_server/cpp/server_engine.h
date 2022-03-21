#pragma once

#include "global_declare.h"

FORWARD_DECLARE_PTR(GrpcServer);
FORWARD_DECLARE_PTR(DBEnginePool);

class ServerEngine
{
    public:
        ServerEngine();


    private:
        GrpcServerPtr       grpc_server_sptr_{nullptr};
        DBEnginePoolPtr     db_engine_sptr_{nullptr};
};