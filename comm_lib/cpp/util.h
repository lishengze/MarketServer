#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"
#include "comm_type_def.h"
#include "comm_declare.h"

COMM_NAMESPACE_START

string get_kline_topic(string exchange, string symbol);

string get_depth_topic(string exchange, string symbol);

string get_trade_topic(string exchange, string symbol);

string get_depth_jsonstr(const SDepthQuote& depth);

string get_kline_jsonstr(const KlineData& kline);

string get_trade_jsonstr(const TradeData& trade);

ReqTradeData get_req_trade(const PReqTradeInfo& proto_reqtrade);

COMM_NAMESPACE_END