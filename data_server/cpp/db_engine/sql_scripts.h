#pragma once

#include "../global_declare.h"

inline string get_kline_table_name(const string& exchange, const string& symbol)
{
    return "kline_" + exchange + "_" + symbol;
}