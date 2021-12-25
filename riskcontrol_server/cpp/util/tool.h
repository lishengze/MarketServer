#pragma once

#include "base/cpp/basic.h"
#include "../data_struct/data_struct.h"

void print_quote(const SDepthQuote& quote);

string quote_str(const SDepthQuote& quote, int count= 0);

bool filter_zero_volume(SDepthQuote& quote, std::mutex& mutex_);

bool filter_zero_volume(SDepthQuote& quote);

bool check_exchange_volume(const SDepthQuote& quote);

string get_work_dir_name(string program_full_name);

string get_program_name(string program_full_name);