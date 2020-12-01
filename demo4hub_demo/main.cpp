#include "hub_interface.h"
#include <thread>

#include <iostream>
using std::cout;
using std::endl;

class Client : public HubCallback
{
public:   
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { 

        cout << "on_depth " << endl;
        return 0;
    } 

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const KlineData& kline) { 
        cout << "on_kline " << endl;
        return 0; 
    }
};

int main(){

    Client client;
    HubInterface::set_callback(&client);
    HubInterface::start();

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}