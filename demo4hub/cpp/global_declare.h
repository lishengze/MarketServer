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

using namespace std;
using nlohmann::json;
using std::pair;
using boost::weak_ptr;


