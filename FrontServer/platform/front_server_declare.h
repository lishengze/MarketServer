#pragma once

#include "envdeclare.h"
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
#include "pandora/util/json.hpp"
#include "pandora/util/thread_basepool.h"
#include "quark/cxx/assign.h"
#include "pandora/package/package_station.h"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "pandora/package/package_manager.h"
#include "pandora/util/thread_safe_singleton.hpp"

#include <iostream>

#define PACKAGE_MANAGER utrade::pandora::ThreadSafeSingleton<utrade::pandora::PackageManager>::DoubleCheckInstance()

using namespace std;
using nlohmann::json;
using std::pair;
using boost::weak_ptr;