#pragma once
#include "../pandora_declare.h"
#include <math.h>

#define PRECISION 0.000000001

PANDORA_NAMESPACE_START

inline bool equal(const double& a, const double& b)
{
    if (fabs(a-b) < PRECISION)
        return true;
    return false;
}

inline bool great(double a, double b)
{
    if (!equal(a,b) && a>b)
        return true;
    return false;
}

inline bool less(double a, double b)
{
    if (!equal(a, b) && a<b)
        return true;
    return false;
}

inline bool great_equal(double a, double b)
{
    if (equal(a,b) || a>b)
        return true;
    return false;
}

inline bool less_equal(double a, double b)
{
    if (equal(a, b) || a<b)
        return true;
    return false;
}

inline void substract(double& a, const double& b)
{
    a -= b;
    if (equal(a, 0))    a = 0;
}

inline void add(double& a, const double& b)
{
    a += b;
    if(equal(a, 0)) a=0;
}

PANDORA_NAMESPACE_END