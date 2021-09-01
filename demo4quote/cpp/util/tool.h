#pragma once

#include "../global_declare.h"
#include "../redis_quote_define.h"

void print_quote(const SDepthQuote& quote);

// string quote_str(const SDepthQuote& quote);


string quote_str(const SDepthQuote& quote, int count=0);

bool filter_zero_volume(SDepthQuote& quote);

bool filter_zero_volume(SEData& sedata);

void print_sedata(const SEData& sedata);

string sedata_str(const SEData& sedata);

string klines_str(vector<KlineData>& kline_list);

string kline_str(KlineData& kline);

inline string get_sec_time_str(unsigned long time);

void  filter_kline_data(vector<KlineData>& kline_list);

bool filter_kline_atom(KlineData& kline);