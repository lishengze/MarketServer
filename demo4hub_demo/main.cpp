#include "hub_interface.h"
#include "base/cpp/pl_decimal.h"
#include "pandora/util/time_util.h"
#include <thread>

#include <iostream>
using std::cout;
using std::endl;
using namespace dec;

class Client : public HubCallback
{
public:   
    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        // return -1;
        // cout << "Test Client on_depth " << depth.ask_length << ", " << depth.bid_length << endl;

        for (int i =0; i < 10; ++i)
        {
            // cout << depth.asks[i].price.get_value() << ", " << depth.bids[i].price.get_value() << endl;
        }

        return 0;
    } 

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { 
        // return -1;
        // cout << "Test Client on_kline: " << klines.size() << endl;
        for( int i = 0 ; i < klines.size() ; i ++ )
        {
            cout << symbol << " " << utrade::pandora::ToSecondStr(klines[i].index*1000*1000*1000, "%Y-%m-%d %H:%M:%S") << " " << klines[i].px_open.get_str_value() << " " << endl;
        }
        return 0; 
    }
};

void test_get_kline()
{
    type_tick end_time_secs = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);    

    type_tick start_time_secs = end_time_secs - 60 * 60;

    vector<KlineData> kline_data;

    HubInterface::get_kline("HUOBI", "BTC_USDT", 1, start_time_secs, end_time_secs, kline_data);

    cout << "Test Get Kline, Size:  " << kline_data.size() << endl;

    for (KlineData& kline:kline_data)
    {
        cout << utrade::pandora::ToSecondStr(kline.index * 1000*1000*1000, "%Y-%m-%d %H:%M:%S") << ", "
             << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
             << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << "\n";        
    }
}

int main()
{
    /*
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
    */
    Client client;
    HubInterface::set_callback(&client);
    HubInterface::start();

    test_get_kline();

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}