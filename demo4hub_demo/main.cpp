#include "hub_interface.h"
#include <thread>

class Client : public HubCallback
{
public:   
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) { 
        cout << exchange << " " << symbol << " " << depth.ask_length << " " << depth.bid_length << endl;
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

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}