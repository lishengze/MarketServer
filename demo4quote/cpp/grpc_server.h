
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "stream_engine_server.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using trade::service::v1::StreamEngineService;
using trade::service::v1::GetQuoteReq;
using trade::service::v1::SubscribeQuoteReq;
using trade::service::v1::QuoteData;
using trade::service::v1::DepthLevel;
using trade::service::v1::Decimal;
using trade::service::v1::DepthVolume;


class ServiceImpl final : public StreamEngineService::Service {
public:    
    void RunServer() {
        std::string server_address("0.0.0.0:50051");

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(this);
        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        server->Wait();
    }

    Status GetQuote(ServerContext* context, const GetQuoteReq* req, QuoteData* quote) override {
        return Status::OK;
    }

    Status SubscribeQuote(ServerContext* context, const SubscribeQuoteReq* req, ServerWriter<QuoteData>* writer) override {
        return Status::OK;
    }

private:
};