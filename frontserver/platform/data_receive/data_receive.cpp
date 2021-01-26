#include "data_receive.h"
#include "../front_server_declare.h"
#include "quark/cxx/assign.h"
#include "pandora/util/time_util.h"
#include "pandora/package/package.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../log/log.h"

#include <chrono>
#include <sstream>

DataReceive::DataReceive(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{

}

DataReceive::~DataReceive()
{
    if (!test_kline_thread_)
    {
        if (test_kline_thread_->joinable())
        {
            test_kline_thread_->join();
        }
    }

    if (!test_enquiry_thread_)
    {
        if (test_enquiry_thread_->joinable())
        {
            test_enquiry_thread_->join();
        }
    }
}

void DataReceive::launch()
{
    cout << "DataReceive::launch " << endl;

    init_grpc_interface();

    test_main();    
}

void DataReceive::init_grpc_interface()
{
    HubInterface::set_callback(this);
    HubInterface::start();
}

void DataReceive::test_main()
{
    // test_rsp_package();

    if (is_test_enquiry)
    {
        test_enquiry_thread_ = std::make_shared<std::thread>(&DataReceive::test_enquiry, this);
    }

    if (is_test_kline)
    {
        test_kline_thread_ = std::make_shared<std::thread>(&DataReceive::test_kline_data, this);
    }
    
}

void DataReceive::test_kline_data()
{
    std::vector<string> symbol_list{"BTC_USDT", "XRP_USDT"};
    int frequency_secs = 60;
    type_tick end_time_secs = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
    end_time_secs = mod_secs(end_time_secs, frequency_secs);

    int test_time_numb = 60 * 2;

    double test_max = 100;
    double test_min = 10;
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(test_min, test_max);    
    std::uniform_real_distribution<> offset(1,10);

    for (int i = 0; i < test_time_numb; ++i)
    {
        type_tick cur_time = end_time_secs - (test_time_numb-i) * frequency_secs;

        double open = dis(gen);
        double close = dis(gen);
        double high = std::max(open, close) + offset(gen);
        double low = std::min(open, close) - offset(gen);
        double volume = dis(gen) * 5;

        for (auto symbol:symbol_list)
        {
            KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

            std::vector<KlineData> vec_kline{*kline_data};

            handle_kline_data("", symbol.c_str(), -1, vec_kline);

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(60));

        type_tick cur_time = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
        end_time_secs = mod_secs(end_time_secs, frequency_secs);

        double open = dis(gen);
        double close = dis(gen);
        double high = std::max(open, close) + offset(gen);
        double low = std::min(open, close) - offset(gen);
        double volume = dis(gen) * 5;

        for (auto symbol:symbol_list)
        {

            KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

            std::vector<KlineData> vec_kline{*kline_data};

            handle_kline_data("", symbol.c_str(), -1, vec_kline);   

            cout << "Update: " <<  get_sec_time_str(cur_time) << " " << symbol << ", "
                << "open: " << open << ", high: " << high << ", "
                << "low: " << low << ", close: " << volume << "\n" << endl;               
        }         
    }
}

void DataReceive::test_rsp_package()
{
    while(true)
    {
        cout << "Send Message " << endl;

        PackagePtr package_new = PACKAGE_MANAGER->AllocateRtnDepth();

        auto p_rtn_depth = GET_NON_CONST_FIELD(package_new, CUTRtnDepthField);
        package_new->prepare_response(UT_FID_RtnDepth, ID_MANAGER->get_id());

        assign(p_rtn_depth->ExchangeID, "HUOBI");
        assign(p_rtn_depth->LocalTime, utrade::pandora::NanoTimeStr());

        deliver_response(package_new);  

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void DataReceive::test_enquiry()
{
    std::vector<string> symbol_list{"BTC_USDT", "XRP_USDT"};

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        SDepthData test_raw_depth;

        test_raw_depth.tick = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
        test_raw_depth.ask_length = 50;
        test_raw_depth.bid_length = 50;

        test_raw_depth.is_raw = true;
        for (int i = 0; i < test_raw_depth.ask_length; ++i)
        {
            test_raw_depth.asks[i].price = 2000 - i * 10;
            test_raw_depth.asks[i].volume = 5;
        }
        for (int i = 0; i < test_raw_depth.bid_length; ++i)
        {
            test_raw_depth.bids[i].price = 1000 - i * 10;
            test_raw_depth.bids[i].volume = 5;
        }        

        for (auto symbol:symbol_list)
        {
            assign(test_raw_depth.symbol, symbol);
            handle_raw_depth("", symbol.c_str(), test_raw_depth);              
        }         
    }
}

void DataReceive::release()
{

}

void DataReceive::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataReceive::handle_request_message, this, package));
}

void DataReceive::response_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataReceive::handle_response_message, this, package));
}

void DataReceive::handle_request_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_ReqKLineData:
            request_kline_package(package);
            return;
        default:
            break;
    }
}

void DataReceive::request_kline_package(PackagePtr package)
{
    try
    {
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);
        if (pReqKlineData)
        {

        }
        else
        {
            LOG_ERROR("DataReceive::request_kline_package ReqKLineData NULL!");
        }        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] DataReceive::request_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void DataReceive::handle_response_message(PackagePtr package)
{

}

// 深度数据（推送）
int DataReceive::on_depth(const char* exchange, const char* symbol, const SDepthData& depth)
{   
    if (is_test_depth)
    {
        return -1;
    }    
    get_io_service().post(std::bind(&DataReceive::handle_depth_data, this, exchange, symbol, depth));
    return 1;
}

// K线数据（推送）
int DataReceive::on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines)
{    
    if (is_test_kline)
    {
        return -1;
    }
    get_io_service().post(std::bind(&DataReceive::handle_kline_data, this, exchange, symbol, resolution, klines));
    return 1;
}

// 原始深度数据推送
// int DataReceive::on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth)
// {
//     return 1;
//     if (is_test_enquiry)
//     {
//         return 1;
//     }
//     get_io_service().post(std::bind(&DataReceive::handle_raw_depth, this, exchange, symbol, depth));
//     return 1;
// }

void DataReceive::handle_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth)
{    
    if (strlen(symbol) == 0) 
    {
        LOG_ERROR("DataReceive::handle_raw_depth symbol is null! \n");
        return;
    }

    if (strlen(exchange) != 0)
    {
        // 只处理聚合数据;
        return;
    }

    std::stringstream stream_obj;
    stream_obj  << "[Depth] handle_raw_depth " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length;
    LOG_DEBUG(stream_obj.str());

    // cout << "Ask: length: " << depth.ask_length << endl;
    // for ( int i = 0; i < depth.ask_length; ++i)
    // {
    //     cout << depth.asks[i].price.get_value() << ", " << depth.asks[i].volume.get_value() << endl;
    // }

    // cout << "\nBid, length: " << depth.bid_length << endl;
    // for ( int i = 0; i < depth.bid_length; ++i)
    // {
    //     cout << depth.bids[i].price.get_value() << ", " << depth.bids[i].volume.get_value() << endl;
    // }    


    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    SDepthData* p_sdepth_data = GET_NON_CONST_FIELD(package, SDepthData);
    p_sdepth_data->is_raw = true;

    package->prepare_response(UT_FID_SDepthData, ID_MANAGER->get_id());

    deliver_response(package);
}

void DataReceive::handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth)
{
    
    if (strlen(symbol) == 0) 
    {
        LOG_ERROR("DataReceive::handle_depth_data symbol is null! \n");
        return;
    }

    if (strlen(exchange) != 0)
    {
        // 只处理聚合数据;
        return;
    }

    std::stringstream stream_obj;
    stream_obj  << "[Depth] handle_depth_data " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length;
    
    // cout << "Ask: length: " << depth.ask_length << endl;
    // for ( int i = 0; i < depth.ask_length; ++i)
    // {
    //     cout << depth.asks[i].price.get_value() << ", " << depth.asks[i].volume.get_value() << endl;
    // }

    // cout << "\nBid, length: " << depth.bid_length << endl;
    // for ( int i = 0; i < depth.bid_length && i < 5; ++i)
    // {
    //     cout << depth.bids[i].price.get_value() << ", " << depth.bids[i].volume.get_value() << endl;
    // }    

    LOG_INFO(stream_obj.str());
    
    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    SDepthData* p_sdepth_data = GET_NON_CONST_FIELD(package, SDepthData);
    
    p_sdepth_data->is_raw = false;
    
    deliver_response(package);
}

void DataReceive::handle_kline_data(const char* exchange, const char* c_symbol, type_resolution resolution, const vector<KlineData>& klines)
{
    if (strlen(c_symbol) == 0) 
    {
        LOG_ERROR ("DataReceive::handle_kline_data symbol is null!");
        return;
    }        

    if (strlen(exchange) != 0)
    {
        return;
    }

    std::stringstream stream_obj;
    stream_obj  << "[Kline] handle_kline_data " << c_symbol << " " << klines.size();
    LOG_DEBUG(stream_obj.str());

    string symbol = string(c_symbol);
    
    for( int i = 0 ; i < klines.size() ; i ++ )
    {
        const KlineData& kline = klines[i];

        // 过滤掉逆序的时间;
        if (kline_symbol_last_time_.find(symbol)== kline_symbol_last_time_.end())
        {
            kline_symbol_last_time_[symbol] == kline.index;
        }
        else if (kline_symbol_last_time_[symbol] >= kline.index)
        {
            std::stringstream stream_obj;
            stream_obj  << "[Kine] Time Seq is Error , "<< symbol << " current time is " << get_sec_time_str(kline.index)
                        << ", last update time is " << get_sec_time_str(kline_symbol_last_time_[symbol]) << "\n";

            // LOG_ERROR(stream_obj.str());
            continue;
        }
        else
        {
           kline_symbol_last_time_[symbol] = kline.index;
        }
        
        // kline.frequency_ = resolution;

        std::stringstream stream_obj;
        stream_obj  << "[Kine] SRC " << get_sec_time_str(kline.index) << " "<< exchange << " " << symbol << ", "
                    << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                    << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value();
        
        // LOG_INFO(stream_obj.str());

        PackagePtr package = GetNewKlineDataPackage(kline, ID_MANAGER->get_id());

        package->prepare_response(UT_FID_KlineData, ID_MANAGER->get_id());

        deliver_response(package);
    }   
}

