#pragma once

#include "../global_declare.h"
#include "rpc.h"

class GrpcServer
{
public:
    GrpcServer(string address):address_{address} {}

    virtual ~GrpcServer();

    void start();

    void init_cq_thread();

    void run_cq_loop();

public:
    void reconnect_rpc(BaseRPC* rpc);

private:
    string                                        address_;

    std::unique_ptr<grpc::ServerCompletionQueue>  cq_;
    std::unique_ptr<grpc::Server>                 server_;
    grpc::ServerBuilder                           builder_;

    std::thread                                   cq_thread_;

private:

};