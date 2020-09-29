#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
//#include "helper.h"
#ifdef BAZEL_BUILD
#include "api.grpc.pb.h"
#else
#include "api.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using trade::service::v1::MarketStreamData;
using trade::service::v1::Depth;
using trade::service::v1::DepthData;
using trade::service::v1::EmptyReply;
using trade::service::v1::Trade;


class TradeClient {
 public:
  TradeClient(std::shared_ptr<Channel> channel)
      : stub_(Trade::NewStub(channel)) {
  }

  void PutMarketStream() {      
    ClientContext context;
    google::protobuf::Empty response;

    std::shared_ptr<ClientWriter<MarketStreamData>> stream(
        stub_->PutMarketStream(&context, &response)
    );

    MarketStreamData msd;
    stream->Write(msd);
    stream->WritesDone();
  }

private:
  std::unique_ptr<Trade::Stub> stub_;
};

class GrpcPublisher {
public:
    void init() {
        TradeClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
        client.PutMarketStream();
    }
};