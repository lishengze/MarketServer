#pragma once

#include <string>
#include <map>
#include <unordered_map>
using namespace std;

using TExchange = string;
using TSymbol = string;
using TQuoteData = string;


#define PRICE_PRECISE 0.000000001
#define TICK_HEAD "TRADEx|"
#define DEPTH_UPDATE_HEAD "UPDATEx|"
#define GET_DEPTH_HEAD "DEPTHx|"
