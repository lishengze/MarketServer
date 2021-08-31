#pragma once

#include "../global_declare.h"
#include "../data_struct/data_struct.h"

void print_quote(const SInnerQuote& quote);

string quote_str(const SInnerQuote& quote);

bool filter_zero_volume(SInnerQuote& quote, std::mutex& mutex_);

bool filter_zero_volume(SInnerQuote& quote);

string get_work_dir_name(string program_full_name);

string get_program_name(string program_full_name);