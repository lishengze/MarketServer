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
using namespace std;

#define _println_(...)                                             \
    do {                                                           \
        char content[1024];                                        \
        sprintf(content, __VA_ARGS__);                             \
        std::cout << fixed << content << std::endl;                         \
    } while(0)

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
