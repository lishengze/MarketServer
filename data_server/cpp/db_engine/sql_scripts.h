#pragma once

#include "../global_declare.h"
#include "../Log/log.h"

inline string get_kline_table_name(const string& exchange, const string& symbol)
{
    return "kline_" + exchange + "_" + symbol;
}

inline string get_qm_string(int qm_count)
{
    try
    {
        string result = "";

        for (int i = 0; i < qm_count; ++i)
        {
            result += " ?,";
        }
        result.erase(result.length()-1);
        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr <<"get_qm_string " << e.what() << '\n';
    }
    return "";
}

inline string char_to_string(char ori_char)
{
    try
    {
        std::stringstream s_obj;
        s_obj << ori_char;
        return s_obj.str();
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DBEngine::insert_req_create_order " << e.what() << '\n';
    }    
    return "";
}

inline string get_create_kline_sql_str(const string& exchange, const string& symbol)
{
    try
    {
        string result = "";
        string table_name = get_kline_table_name(exchange, symbol);
        result += "CREATE TABLE IF NOT EXISTS " +  table_name +  string("( \
                    exchange VARCHAR(32), \
                    symbol VARCHAR(64), \
                    resolution BIGINT,\
                    time BIGINT, \
                    open DECIMAL(32, 8), \
                    high DECIMAL(32, 8), \
                    low DECIMAL(32, 8), \
                    close DECIMAL(32, 8), \
                    volume DECIMAL(32, 8) \
                    PRIMARY KEY (`time`)) DEFAULT CHARSET utf8;");

        // cout << "create sql: " << result << endl;

        return result; 
    }
    catch(const std::exception& e)
    {
        std::cerr << "get_req_create_order_sql_str: " << e.what() << '\n';
    }
    return "";
}