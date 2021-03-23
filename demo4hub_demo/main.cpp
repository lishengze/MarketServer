#include "hub_interface.h"
#include "base/cpp/pl_decimal.h"
#include "base/cpp/tinyformat.h"
#include "pandora/util/time_util.h"
#include <thread>

#include <iostream>
using std::cout;
using std::endl;
using namespace dec;

class PressurePerformanceClient : public HubCallback
{
public:
    PressurePerformanceClient() {
        stopped_ = true;
        
        delays_.reserve(1024*1024);
        trade_cnt_ = 0;

        delays_raw_.reserve(1024*1024);
        trade_raw_cnt_ = 0;

        last_output_time_ = get_miliseconds() / 1000;
    }
private:
    bool stopped_;
    // 聚合行情统计
    vector<type_tick> delays_;  
    uint64 trade_cnt_;
    // 原始行情统计
    vector<type_tick> delays_raw_;
    uint64 trade_raw_cnt_;
    // 临时变量
    type_tick last_output_time_;

public:
    // 风控前数据（推送）
    virtual int on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        if( stopped_ )
            return 0;

        type_tick now = get_miliseconds();
        type_tick delay = now - depth.tick1;
        delays_raw_.push_back(delay);
        if( string(exchange) == "BINANCE" && string(symbol) == "BTC_USDT" && (now/1000 - last_output_time_) >= 1) {
            _print("update: %s.%s %s bias=%lu", exchange, symbol, FormatISO8601DateTime(depth.tick/1000000000), now/1000 - depth.tick/1000000000);
            last_output_time_ = now / 1000;
        }
        return 0;
    }

    // 风控后数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        if( stopped_ )
            return 0;

        cout << exchange << " " << symbol << " " << depth.ask_length << " / " <<  depth.bid_length << std::endl;

        type_tick now = get_miliseconds();
        type_tick delay = now - depth.tick1;
        delays_.push_back(delay);
        return 0;
    } 

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines) { return 0; }

    // trade（推送）
    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade) { 
        if( stopped_ )
            return 0;
        if( string(exchange) == MIX_EXCHANGE_NAME ) {
            trade_cnt_ ++;
        } else {
            trade_raw_cnt_ ++;
        }
        return 0; 
    }

    void _print_statistics(vector<type_tick>& delays, uint64& trade_cnt)
    {
        // 排序
        sort(delays.begin(), delays.end());

        // total
        tfm::printfln("总共%u次行情更新", delays.size());
        tfm::printfln("总共%u笔成交", trade_cnt);
        // 平均值
        type_tick total = 0, max_value = 0, min_value = 999999;
        for( auto v : delays ) {
            total += v;
            if( v > max_value ) {
                max_value = v;
            }
            if( v < min_value ) {
                min_value = v;
            }
        }
        tfm::printfln("平均值%lums", total / delays.size());
        // 中位数
        tfm::printfln("中位数%lums", delays[delays.size() / 2]);
        // 最大值
        tfm::printfln("最大值%lums", max_value);
        // 最小值
        tfm::printfln("最小值%lums", min_value);
        // 85%位置值
        tfm::printfln("85%%位置值%lums", delays[int(delays.size() * 0.85)]);
        // 90%位置值
        tfm::printfln("90%%位置值%lums", delays[int(delays.size() * 0.9)]);
        // 95%位置值
        tfm::printfln("95%%位置值%lums", delays[int(delays.size() * 0.95)]);
    }

    void print_debug() {
        // 延迟10秒启动
        std::this_thread::sleep_for(std::chrono::seconds(10));
        stopped_ = false;
        // 采样30秒
        std::this_thread::sleep_for(std::chrono::seconds(60));
        stopped_ = true;
        // 等待1秒停止完成
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 排序
        tfm::printfln("输出聚合行情统计结果：");
        _print_statistics(delays_, trade_cnt_);
        tfm::printfln("输出原始行情统计结果：");
        _print_statistics(delays_raw_, trade_raw_cnt_);
    }
};

class Client : public HubCallback
{
public:   
    void print_debug() {}
    
    // 风控前数据（推送）
    virtual int on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        /*
        type_tick now = get_miliseconds();
        type_tick delay = now-depth.tick1;
        //if( delay > 100 )
            tfm::printfln("[raw_depth] %s.%s delay=%u ask_depth=%u bid_depth=%u.( se_cost=%u tick=%u tick1=%u tick2=%u )", 
                exchange, symbol, delay, depth.ask_length, depth.bid_length, depth.tick2 - depth.tick1,
                depth.tick, depth.tick1, depth.tick2);
        */
        return 0;
    } 

    // 风控后数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth) 
    { 
        /*
        type_tick now = get_miliseconds();
        type_tick delay = now-depth.tick1;
        //if( delay > 5000 )
            tfm::printfln("[depth] %s.%s delay=%u ask_depth=%u bid_depth=%u.( se_cost=%u rc_cost=%u)", 
                exchange, symbol, delay, depth.ask_length, depth.bid_length, depth.tick2 - depth.tick1, depth.tick3 - depth.tick2);        
        */
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
        if( string(exchange) == "_bcts_" && string(symbol) == "BTC_USDT" )
        {
            for( int i = 0 ; i < klines.size() ; i ++ )
            {
                tfm::printfln("[kline %u] %s.%s index=%s(%u) open=%s high=%s low=%s close=%s volume=%s", resolution, exchange, symbol, 
                    utrade::pandora::ToSecondStr(klines[i].index*1000*1000*1000, "%Y-%m-%d %H:%M:%S"), klines[i].index,
                    klines[i].px_open.get_str_value(), klines[i].px_high.get_str_value(), klines[i].px_low.get_str_value(),
                    klines[i].px_close.get_str_value(), klines[i].volume.get_str_value());
            }
        }
        /*
        if( klines.size() > 5 )
            return 0;
        for( int i = 0 ; i < klines.size() ; i ++ )
        {
            tfm::printfln("[kline %u] %s.%s index=%s(%u) open=%s high=%s low=%s close=%s volume=%s", resolution, exchange, symbol, 
                utrade::pandora::ToSecondStr(klines[i].index*1000*1000*1000, "%Y-%m-%d %H:%M:%S"), klines[i].index,
                klines[i].px_open.get_str_value(), klines[i].px_high.get_str_value(), klines[i].px_low.get_str_value(),
                klines[i].px_close.get_str_value(), klines[i].volume.get_str_value());
                
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
        */
        return 0; 
    }

    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade) 
    {
        //if( string(exchange) == MIX_EXCHANGE_NAME )
        //tfm::printfln("[trade] %s.%s time=%lu price=%s volume=%s", exchange, symbol, trade.time, trade.price.get_str_value(), trade.volume.get_str_value());
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
    //Client client;
    PressurePerformanceClient client;
    HubInterface::set_callback(&client);
    HubInterface::start();

    // test_get_kline();
    // test_get_lasttrades();

    client.print_debug();

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}