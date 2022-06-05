#include "base/cpp/basic.h"

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "market_data.pb.h"
#include "market_data.grpc.pb.h"

#include "risk_controller.pb.h"
#include "risk_controller.grpc.pb.h"

using grpc::Alarm;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;

using Proto3::MarketData::ReqTradeInfo;
using Proto3::MarketData::TradeData;
using Proto3::MarketData::MarketService;

using quote::service::v1::RiskController;
using quote::service::v1::QuoteRequest;
using quote::service::v1::QuoteResponse;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;