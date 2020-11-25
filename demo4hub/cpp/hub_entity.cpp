#include "hub_entity.h"
#include "hub_interface.h"

HubEntity::HubEntity()
{

}

HubEntity::~HubEntity()
{

}

int HubEntity::start()
{
    return 0;
}

int HubEntity::stop()
{
    return 0;
}

int HubEntity::get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    return 0;
}

