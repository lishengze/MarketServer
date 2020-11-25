#include "hub_interface.h"

class Client : public HubCallback
{
public:   
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { 
        return 0;
    } 

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const KlineData& kline) { 
        return 0; 
    }
};

int main(){

    Client client;
    HubInterface::set_callback(&client);
    HubInterface::start();
    return 0;
}