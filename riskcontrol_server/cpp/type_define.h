#pragma once

#define TOPIC_SEPARATOR "|"
#define TYPE_SEPARATOR "-"
#define SYMBOL_EXCHANGE_SEPARATOR "."



#define TRADE_HEAD "TRADEx"
#define DEPTH_UPDATE_HEAD "UPDATEx"
#define GET_DEPTH_HEAD "DEPTHx"
#define KLINE_1MIN_HEAD "KLINEx"
#define KLINE_60MIN_HEAD "SLOW_KLINEx"
#define SNAP_HEAD "__SNAPx" // 为了统一接口，自定义的消息头

#define KLINE_TYPE KLINE_1MIN_HEAD
#define DEPTH_TYPE GET_DEPTH_HEAD
#define TRADE_TYPE TRADE_HEAD

