#pragma once

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <thread>
#include <future>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <chrono>
#include <mutex>
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

// json
#include "base/cpp/rapidjson/document.h"
#include "base/cpp/rapidjson/writer.h"
#include "base/cpp/rapidjson/stringbuffer.h"

#include "type_define.h"

using namespace std;

#define DECLARE_PTR(X) typedef boost::shared_ptr<X> X##Ptr     /** < define smart ptr > */
#define FORWARD_DECLARE_PTR(X) class X; DECLARE_PTR(X)         /** < forward defile smart ptr > */
