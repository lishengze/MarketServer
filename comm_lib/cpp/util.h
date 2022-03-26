#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"
#include "comm_type_def.h"
#include "comm_declare.h"
#include "comm_log.h"

COMM_NAMESPACE_START

inline void set_decimal(PDecimal* dst, const SDecimal& src)
{
    dst->set_precise(src.prec());
    dst->set_value(src.value());
}

string get_kline_topic(string exchange, string symbol);

string get_depth_topic(string exchange, string symbol);

string get_trade_topic(string exchange, string symbol);

string get_depth_jsonstr(const SDepthQuote& depth);

string get_kline_jsonstr(const KlineData& kline);

string get_trade_jsonstr(const TradeData& trade);

ReqTradeData get_req_trade(const PReqTradeInfo& proto_reqtrade);


void init_log(string program_name="comm", string work_dir="");

COMM_NAMESPACE_END