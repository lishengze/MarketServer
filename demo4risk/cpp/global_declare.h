#pragma once

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <thread>
#include <future>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <cstring>
#include <mutex>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <chrono>
#include <list>
#include <vector>
#include <queue>
#include <condition_variable>

#include "stream_engine.grpc.pb.h"
#include "stream_engine.pb.h"

#include "risk_controller.grpc.pb.h"
#include "risk_controller.pb.h"

#include "account.grpc.pb.h"
#include "account.pb.h"

#include "quote_data.pb.h"

#include "base/cpp/basic.h"
#include "base/cpp/quote.h"
#include "base/cpp/decimal.h"

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
using quote::service::v1::DataInBinary;

using quote::service::v1::TradedOrderStreamData_Direction;

// using 

using namespace std;

#define DECLARE_PTR(X) typedef boost::shared_ptr<X> X##Ptr     /** < define smart ptr > */
#define FORWARD_DECLARE_PTR(X) class X; DECLARE_PTR(X)         /** < forward defile smart ptr > */
