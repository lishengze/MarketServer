#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>
#include <string>
#include <map>
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
        std::cout << std::fixed << content << std::endl;                         \
    } while(0)                                                          

template<class T,class S>
inline void vassign(T &r, S v)
{
	r = v;
}

template<class T>
inline void vassign(T &r, const T v)
{
	r = v;
}

inline void vassign(char * r, unsigned int len, const char *v)
{
    unsigned int l = std::min(len-1, (unsigned int)strlen(v));
	strncpy(r, v, l);
    r[l] = '\0';
}

inline void vassign(char * r, unsigned int len, const std::string &v)
{
    unsigned int l = std::min(len-1, (unsigned int)v.length());
    strncpy(r, v.c_str(), l);
    r[l] = '\0';
}

#define MAX_EXCHANGE_LENGTH 10
#define MAX_DEPTH_LENGTH 200
#define MAX_SYMBOLNAME_LENGTH 32
#define MAX_EXCHANGENAME_LENGTH 32