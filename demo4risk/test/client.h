#pragma once

#include <iostream>

#include "global_declare.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>

#include "risk_controller.grpc.pb.h"
#include "risk_controller.pb.h"

using grpc::Alarm;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientAsyncReader;
using grpc::ClientReader;

using grpc::ClientReaderWriter;
using grpc::ClientAsyncReaderWriter;

using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using quote::service::v1::MultiMarketStreamData;
using quote::service::v1::MarketStreamData;
using quote::service::v1::Depth;
using quote::service::v1::MultiMarketStreamDataWithDecimal;
using quote::service::v1::MarketStreamDataWithDecimal;
using quote::service::v1::DepthWithDecimal;
using quote::service::v1::Decimal;
using quote::service::v1::QuoteRequest;
using quote::service::v1::QuoteResponse;
using quote::service::v1::GetParamsResponse;
using quote::service::v1::TradedOrderStreamData;

using quote::service::v1::TradedOrderStreamData;

using GrpcRiskControllerService = quote::service::v1::RiskController;

class SyncClient
{
public:
    SyncClient(std::shared_ptr<Channel> channel):
        stub_{GrpcRiskControllerService::NewStub(channel)}
    {

    }

    void PutOrderStream();

    std::unique_ptr<GrpcRiskControllerService::Stub>  stub_{nullptr};    
};