#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <string.h>
#include <random>
#include <algorithm>
#include <mutex>
#include <iostream>

#include "pandora/package/package_simple.h"
#include "pandora/util/json.hpp"
#include "pandora/util/thread_basepool.h"
#include "pandora/util/thread_safe_singleton.hpp"


#include "quark/cxx/assign.h"

#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "common_datatype_define.h"

#include "util/id.hpp"
#include "App.h"
#include "libusockets.h"

#define PACKAGE_MANAGER utrade::pandora::ThreadSafeSingleton<utrade::pandora::PackageManager>::DoubleCheckInstance()
#define ID_MANAGER utrade::pandora::ThreadSafeSingleton<ID>::DoubleCheckInstance()

using namespace std;
using nlohmann::json;
using std::pair;
using boost::weak_ptr;

using HttpResponse = uWS::HttpResponse<false> ;
using HttpRequest = uWS::HttpRequest;
using WebsocketClass = uWS::WebSocket<false, true>;

#define MARKET_DATA_UPDATE "market_data_update"
#define SYMBOL_LIST "symbol_list"
#define SYMBOL_UPDATE "symbol_update"
#define KLINE_RSP "kline_rsp"
#define KLINE_UPDATE "kline_update"
#define HEARTBEAT "heartbeat"

#define TRADE "trade"

#define RSP_ENQUIRY "enquiry"
#define RSP_ERROR "error"

#define DOT_NUMB 2
#define PERSENT_DOT_NUMB 4


#define MAX_DOUBLE 10000000000000
#define MIN_DOUBLE -10000000000000

#define NanoPerSec 1000 * 1000 * 1000
#define MicroPerSec 1000 * 1000
#define MillPersec 1000

