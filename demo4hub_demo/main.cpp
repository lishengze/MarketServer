#include "hub_interface.h"
#include "base/cpp/pl_decimal.h"
#include "base/cpp/tinyformat.h"
#include "pandora/util/time_util.h"
#include <thread>

#include <iostream>
using std::cout;
using std::endl;
using namespace dec;

class Client : public HubCallback
{
public:   
    // 风控前数据（推送）
    virtual int on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        type_tick now = get_miliseconds();
        type_tick delay = now-depth.tick1;
        if( delay > 50 )
            tfm::printfln("[raw_depth] %s.%s delay=%u ask_depth=%u bid_depth=%u.( se_cost=%u tick=%u tick1=%u tick2=%u )", 
                exchange, symbol, delay, depth.ask_length, depth.bid_length, depth.tick2 - depth.tick1,
                depth.tick, depth.tick1, depth.tick2);
        
        return 0;
    } 

    // 风控后数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        type_tick now = get_miliseconds();
        type_tick delay = now-depth.tick1;
        if( delay > 50 )
            tfm::printfln("[depth] %s.%s delay=%u ask_depth=%u bid_depth=%u.( se_cost=%u rc_cost=%u)", 
                exchange, symbol, delay, depth.ask_length, depth.bid_length, depth.tick2 - depth.tick1, depth.tick3 - depth.tick2);        
        return 0;
    } 

    // K线数据（推送）
    // virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { 
    //     // return -1;
    //     // cout << "Test Client on_kline: " << klines.size() << endl;
    //     // for( int i = 0 ; i < klines.size() ; i ++ )
    //     for (const KlineData& kline:klines)
    //     {
    //         // cout << symbol << " " << utrade::pandora::ToSecondStr(klines[i].index*1000*1000*1000, "%Y-%m-%d %H:%M:%S") << " " << klines[i].px_open.get_str_value() << " " << endl;

    //         // const KlineData& kline = klines[i];
    //         cout <<"[Kline] " << utrade::pandora::ToSecondStr(kline.index * 1000*1000*1000, "%Y-%m-%d %H:%M:%S") << ", "
    //             << kline.symbol << ", "
    //             << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
    //             << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << "\n";    

    //     }
    // }

    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) 
    { 
        for( int i = 0 ; i < klines.size() ; i ++ )
        {
            tfm::printfln("[kline] %s.%s index=%s open=%s", exchange, symbol, utrade::pandora::ToSecondStr(klines[i].index*1000*1000*1000, "%Y-%m-%d %H:%M:%S"), klines[i].px_open.get_str_value());
                
            // 检查K线是否有倒着走            
            if( resolution == 60 ) 
            {
                type_tick last_index = kline1_cache[exchange][symbol];
                type_tick new_index = klines[i].index;
                if( new_index < last_index ) {
                    tfm::printfln("[kline] fatal error.");
                }
                assert( new_index >= last_index );
                kline1_cache[exchange][symbol] = new_index;
            }
        }
        return 0; 
    }

    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade) 
    {
        // tfm::printfln("[trade] %s.%s time=%lu price=%s volume=%s", exchange, symbol, trade.time, trade.price.get_str_value(), trade.volume.get_str_value());
        return 0;
    }

private:
    unordered_map<string, unordered_map<string, type_tick>> kline1_cache;
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

void test_get_lasttrades()
{
    vector<Trade> trades;

    HubInterface::get_lasttrades(trades);

    cout << "Test get_lasttrades, Size:  " << trades.size() << endl;
}

int main()
{
    Client client;
    HubInterface::set_callback(&client);
    HubInterface::start();

    // test_get_kline();
    // test_get_lasttrades();

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}