#pragma once

#include "hub_struct.h"
#include "../data_structure/comm_data.h"
#include "../front_server_declare.h"
#include <random>

void copy_sdepthdata(SDepthData* des, const SDepthData* src);

void copy_klinedata(KlineData* des, const KlineData* src);

void copy_enhanced_data(RspRiskCtrledDepthData* des, const RspRiskCtrledDepthData* src);

string SDepthDataToJsonStr(const SDepthData& depth);

string SymbolsToJsonStr(RspSymbolListData& symbol_data, string type=SYMBOL_UPDATE);

string SymbolsToJsonStr(std::set<std::string> symbols, string type=SYMBOL_LIST);

string RspRiskCtrledDepthDataToJsonStr(RspRiskCtrledDepthData& en_data, string type=MARKET_DATA_UPDATE);

string RspKlinDataToJsonStr(RspKLineData& rsp_kline_data, string type=KLINE_UPDATE);

inline type_tick trans_string_to_type_tick(string time_str)
{
    type_tick result = std::stoul(time_str);
    return result;
}

inline type_tick mod_secs(type_tick ori_time, int frequency_secs)
{
    ori_time = ori_time - ori_time % frequency_secs;
    return ori_time;
}

inline double get_random_double(double max, double min)
{
    std::random_device rd;  // 将用于为随机数引擎获得种子
    std::mt19937 gen(rd()); // 以播种标准 mersenne_twister_engine
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}

inline int get_random_double(int max, int min)
{
    std::random_device rd;  // 将用于为随机数引擎获得种子
    std::mt19937 gen(rd()); // 以播种标准 mersenne_twister_engine
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

string get_sec_time_str(unsigned long time);

void append_kline_to_klinePtr(std::vector<KlineDataPtr>& des, std::vector<KlineData>& src);

string get_error_send_rsp_string(string err_msg);

string get_heartbeat_str();

string set_double_string_scale(string ori_data, int num);

string simplize_string(string ori_data);

string append_zero(string result, int count);