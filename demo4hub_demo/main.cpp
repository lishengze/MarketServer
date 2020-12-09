#include "hub_interface.h"
#include "base/cpp/pl_decimal.h"
#include <thread>

#include <iostream>
using std::cout;
using std::endl;
using namespace dec;

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
        cout << "on_kline " << exchange << " " << symbol << " " << kline.index << " " << kline.px_open.get_str_value() << " " << endl;
        return 0; 
    }
};

int main(){

    char value[1024];
    sprintf(value, "%f", 0.165);
    cout << value << endl;
    SDecimal pp = SDecimal::parse(value);
    cout << pp.get_str_value() << endl;

    decimal<4> v1 = fromString<decimal<4>>("0.6068");
    decimal<3, ceiling_round_policy> v = fromString<decimal<3, ceiling_round_policy>>(toString(v1));
    //decimal<4, floor_round_policy> v = fromString<decimal<4, floor_round_policy>>("0.606");
    //v *= 1.05;
    cout << sizeof(v) << "\t" << v.getDecimalPoints() << "\t" << v.getUnbiased() << "\t" << v.getAsInteger() << "\t" << toString(v) << endl;

    Client client;
    HubInterface::set_callback(&client);
    HubInterface::start();

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}