#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/quote.h"
#include "base/cpp/decimal.h"
#include "base/cpp/base_data_stuct.h"

#include "stream_engine.grpc.pb.h"
#include "stream_engine.pb.h"

#include "risk_controller.grpc.pb.h"
#include "risk_controller.pb.h"

#include "account.grpc.pb.h"
#include "account.pb.h"

#include "quote_data.pb.h"



using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using quote::service::v1::StreamEngine;
using quote::service::v1::SubscribeQuoteReq;
using quote::service::v1::SubscribeMixQuoteReq;
using SEMultiData = quote::service::v1::MultiMarketStreamDataWithDecimal;
using SEData = quote::service::v1::MarketStreamDataWithDecimal;
using SEDepth = quote::service::v1::DepthWithDecimal;
using SEDecimal = quote::service::v1::Decimal;

using quote::service::v1::MarketStreamData;
using quote::service::v1::MarketStreamDataWithDecimal;
using quote::service::v1::QuoteResponse_Result;
using quote::service::v1::DataInBinary;

using quote::service::v1::TradedOrderStreamData_Direction;

// using 

using namespace std;

#define DECLARE_PTR(X) typedef boost::shared_ptr<X> X##Ptr     /** < define smart ptr > */
#define FORWARD_DECLARE_PTR(X) class X; DECLARE_PTR(X)         /** < forward defile smart ptr > */
