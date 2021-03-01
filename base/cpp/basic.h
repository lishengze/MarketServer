#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <sys/time.h>
using namespace std;
#include "tinyformat.h"

#ifdef WIN32
using uint32 = unsigned int;
using uint64 = unsigned long long;
using int64 = long long;
using type_seqno = unsigned long;
using type_tick = unsigned long;
using type_resolution = int;
using type_length = unsigned short;
using type_uint32 = unsigned int;
#else
using uint32 = unsigned int;
using uint64 = unsigned long;
using int64 = long;
using type_seqno = unsigned long;
using type_tick = unsigned long;
using type_resolution = int;
using type_length = unsigned short;
using type_uint32 = unsigned int;
#endif

inline type_tick get_miliseconds() {
    auto time_now = chrono::system_clock::now();
	auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}

inline uint64 ToUint64(const string& s) {
    std::istringstream iss(s);
    uint64 i64;
    iss >> i64;
    return i64;
}

template <typename T>
std::string ToString(const T& t)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << t;
    return oss.str();
}

inline type_tick parse_nano(const string& timestr)
{ // 2020-12-27 12:48:41.578000
    std::string base, remain;
    std::size_t pos = timestr.find(".");
    if( pos == std::string::npos ) {
        base = timestr;
    } else {
        base = timestr.substr(0, pos);
        remain = timestr.substr(pos+1);
    }
    //cout << base << " " << remain << endl;
    
    struct tm _tm = {0};
    strptime(base.c_str(), "%Y-%m-%d %H:%M:%S", &_tm);
    return mktime(&_tm) * 1000000000 + atoi(remain.c_str());
}

class TimeCostWatcher
{
public:
    TimeCostWatcher(const string& desc, type_tick begin=0, type_tick threshold=50)
    : begin_(begin) 
    , threshold_(threshold)
    , desc_(desc)
    {
        if( begin_ == 0 ) {
            begin_ = get_miliseconds();            
        }
    }

    ~TimeCostWatcher() {
        finish();
    }

    void finish() {
        type_tick end = get_miliseconds();
        if( (end-begin_) > threshold_ ) {
            cout << "[TimeCostWatcher] " << desc_ << " cost " << (end-begin_) << endl;
        }
    }

private:
    type_tick begin_;
    type_tick threshold_;
    string desc_;
};

inline std::string FormatISO8601DateTime(int64_t nTime){
    struct tm ts;
    time_t time_val = nTime;
    if (gmtime_r(&time_val, &ts) == nullptr) {
        return {};
    }
    return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

#define _print(fmt, ...)                                             \
    do {                                                             \
        std::string log_msg;                                         \
        try {                                                        \
            log_msg = tfm::format(fmt, ##__VA_ARGS__);               \
        } catch (tinyformat::format_error& fmterr) {                 \
            /* Original format string will have newline so don't add one here */ \
            log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt; \
        }                                                            \
        std::string new_fmt = "%s:%d - " + string(fmt);              \
                                                                     \
        tfm::printfln("%s - %s:%d - %s", FormatISO8601DateTime(get_miliseconds()/1000), __FILE__, __LINE__, log_msg);    \
    } while(0)                                                          
