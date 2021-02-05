#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <sys/time.h>
using namespace std;

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
    TimeCostWatcher(const string& desc)
    : threshold_(50)
    , desc_(desc)
    {
        begin_ = get_miliseconds();
    }

    TimeCostWatcher(const string& desc, type_tick begin)
    : begin_(begin) 
    , threshold_(50)
    , desc_(desc)
    {

    }

    ~TimeCostWatcher() {
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
