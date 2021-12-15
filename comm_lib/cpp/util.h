#pragma once

#include "base/cpp/basic.h"
#include "comm_type_def.h"

COMM_NAMESPACE_START

string get_kline_topic(string exchange, string symbol)
{
    try
    {
        return string(KLINE_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return "";
}


string get_depth_topic(string exchange, string symbol)
{
    try
    {
        return string(DEPTH_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_trade_topic(string exchange, string symbol)
{
    try
    {
        return string(TRADE_TYPE) + TYPE_SEPARATOR + symbol+ SYMBOL_EXCHANGE_SEPARATOR  + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

COMM_NAMESPACE_END