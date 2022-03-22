#pragma once

#include "../global_declare.h"


struct DBConnectInfo
{
    DBConnectInfo() {}
    DBConnectInfo(string usr, string pwd, string schema, int port, string host):
                usr_{usr}, pwd_{pwd}, schema_{schema}, port_{port}, host_{host} 
             {}

    string str()
    {
        return "usr: " + usr_ + ", pwd: " + pwd_ + ", schema: " + schema_
                + ", port: " + std::to_string(port_) + ", host: " + host_;
    }

    string                          usr_;
    string                          pwd_;
    string                          host_;
    string                          schema_;
    int                             port_;
};

struct ReqKlineData
{
    string symbol;
    string exchange;    
    uint64 start_time;
    uint64 end_time;
    uint32 count;
    uint32 frequency;    
};

struct ReqTradeData
{
    ReqTradeData(const PReqTradeInfo& proto_reqtrade)
    {
        symbol = proto_reqtrade.symbol();
        exchange = proto_reqtrade.exchange();
        time = proto_reqtrade.time();
    }

    ReqTradeData() {}


    string symbol;
    string exchange;
    uint64 time;
};