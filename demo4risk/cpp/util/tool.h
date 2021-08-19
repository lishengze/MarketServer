#pragma once

#include "../global_declare.h"
#include "../data_struct/data_struct.h"

void print_quote(const SInnerQuote& quote);

bool filter_zero_volume(SInnerQuote& quote);