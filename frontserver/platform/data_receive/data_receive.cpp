#include "data_receive.h"
#include "../front_server_declare.h"
#include "quark/cxx/assign.h"
#include "pandora/util/time_util.h"
#include "pandora/package/package.h"
#include "../util/tools.h"
#include "../log/log.h"

#include <chrono>
#include <sstream>

DataReceive::DataReceive(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{

}

DataReceive::~DataReceive()
{
    if (!test_thread_)
    {
        if (test_thread_->joinable())
        {
            test_thread_->join();
        }
    }
}

void DataReceive::launch()
{
    cout << "DataReceive::launch " << endl;

    init_grpc_interface();

    if (is_test_)
    {
        test_thread_ = std::make_shared<std::thread>(&DataReceive::test_main, this);
    }    
}

void DataReceive::init_grpc_interface()
{
    HubInterface::set_callback(this);
    HubInterface::start();
}

void DataReceive::test_main()
{
    // test_rsp_package();

    test_kline_data();
}

void DataReceive::test_kline_data()
{
    string symbol = "BTC_USDT";
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

        KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

        std::vector<KlineData> vec_kline{*kline_data};

        handle_kline_data("HUOBI", symbol.c_str(), -1, vec_kline);

        // boost::shared_ptr<KlineData> cur_kline_data = boost::make_shared<KlineData>(symbol, cur_time, open, high, low, close,volume);

        // kline_data_[symbol][cur_time] = new KlineData{symbol, cur_time, open, high, low, close, volume};

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // while (true)
    // {
    //     type_tick cur_time = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
    //     end_time_secs = mod_secs(end_time_secs, frequency_secs);

    //     double open = dis(gen);
    //     double close = dis(gen);
    //     double high = std::max(open, close) + offset(gen);
    //     double low = std::min(open, close) - offset(gen);
    //     double volume = dis(gen) * 5;

    //     KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

    //     std::vector<KlineData> vec_kline{*kline_data};

    //     handle_kline_data("HUOBI", symbol.c_str(), -1, vec_kline);        

    //     cout << get_sec_time_str(cur_time) << "symbol: " << symbol << ", \n"
    //         << "open: " << open << ", high: " << high << ", "
    //         << "low: " << low << ", close: " << volume << endl;

    //     std::this_thread::sleep_for(std::chrono::seconds(60));
    // }
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
    // cout << "on_depth " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << endl;

    // return -1;
    get_io_service().post(std::bind(&DataReceive::handle_depth_data, this, exchange, symbol, depth));
    return 1;
}

// K线数据（推送）
int DataReceive::on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines)
{
    cout << "klines.size: " << klines.size() << endl;
    return -1;
    get_io_service().post(std::bind(&DataReceive::handle_kline_data, this, exchange, symbol, resolution, klines));
    return 1;
}

void DataReceive::handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth)
{
    
    if (strlen(symbol) == 0) 
    {
        LOG_INFO("DataReceive::handle_depth_data symbol is null!");
        return;
    }

    // cout << "handle_depth_data " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << endl;
    
    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    package->prepare_response(UT_FID_SDepthData, ID_MANAGER->get_id());

    deliver_response(package);
}

void DataReceive::handle_kline_data(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines)
{
    // cout << "klines.size: " << klines.size() << endl;
    for( int i = 0 ; i < klines.size() ; i ++ )
    {
        const KlineData& kline = klines[i];

        std::stringstream stream_obj;
        stream_obj  << get_sec_time_str(kline.index) << " symbol: " << symbol << ", "
                    << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                    << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value();
        
        if (strlen(symbol) == 0) 
        {
            LOG_ERROR ("DataReceive::handle_kline_data symbol is null!");
            break;
        }        
        else
        {
            LOG_INFO(stream_obj.str());
        }
        
        PackagePtr package = GetNewKlineDataPackage(kline, ID_MANAGER->get_id());

        package->prepare_response(UT_FID_KlineData, ID_MANAGER->get_id());

        deliver_response(package);
    }   
}

