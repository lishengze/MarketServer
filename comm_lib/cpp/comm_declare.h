#pragma once

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "market_data.pb.h"
#include "market_data.grpc.pb.h"

using grpc::ServerAsyncWriter;
using grpc::ServerContext;


using grpc::Alarm;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;


using PDecimal          = Proto3::MarketData::Decimal;
using PDepth            = Proto3::MarketData::Depth;
using PDepthQuote       = Proto3::MarketData::DepthQuote;
using PKlineData        = Proto3::MarketData::KlineData;
using PTradeData        = Proto3::MarketData::TradeData;
using PRepeatedDepth    = google::protobuf::RepeatedPtrField<PDepth>;
using PDepthMap         = google::protobuf::Map<std::string, Proto3::MarketData::Decimal>;

using Proto3::MarketData::ReqTradeInfo;
using Proto3::MarketData::MarketService;

using PMarketService    = Proto3::MarketData::MarketService::AsyncService;
using PReqTradeInfo     = Proto3::MarketData::ReqTradeInfo;


#define COMM_NAMESPACE_START namespace bcts { namespace comm {
#define COMM_NAMESPACE_END }};
#define USING_COMM_NAMESPACE using namespace bcts::comm;
