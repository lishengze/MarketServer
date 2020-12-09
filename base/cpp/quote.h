#pragma once

#include "basic.h"

#define TSymbol string
#define TExchange string

#define PRICE_PRECISE 0.000000001
#define VOLUME_PRECISE 0.000000001

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
