#pragma once

#include "../global_declare.h"
#include "../redis_quote_define.h"

void print_quote(const SDepthQuote& quote);

string quote_str(const SDepthQuote& quote);

bool filter_zero_volume(SDepthQuote& quote);

bool filter_zero_volume(SEData& sedata);

void print_sedata(const SEData& sedata);

string sedata_str(const SEData& sedata);