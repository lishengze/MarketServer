#include "hub_interface.h"
#include "hub_entity.h"
#include "Log/log.h"

#include <iostream>
using std::cout;
using std::endl;


int HubInterface::start(string file_name) 
{
    LOG_INFO("HubInterface Start");

    return HUB->start(file_name);
}

int HubInterface::stop()
{
    LOG_INFO("HubInterface stop");
    return HUB->stop();
}

int HubInterface::set_callback(HubCallback* callback) 
{
    return HUB->set_callback(callback);
}

// K线数据（请求）
int HubInterface::get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    // cout << "HubInterface get_kline " << endl;

    return HUB->get_kline(exchange, symbol, resolution, start_time, end_time, klines);
}

// 最近成交（请求）
int HubInterface::get_lasttrades(vector<Trade>& trades)
{
    LOG_INFO("HubInterface get_lasttrades");
    return HUB->get_lasttrades(trades);
}