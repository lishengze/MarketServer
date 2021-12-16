#include "util.h"

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
    return "";
}

string get_depth_jsonstr(const SDepthQuote& depth)
{
    try
    {
        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_kline_jsonstr(const KlineData& kline)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_trade_jsonstr(const TradeData& trade)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

COMM_NAMESPACE_END