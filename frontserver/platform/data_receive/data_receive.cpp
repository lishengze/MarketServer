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
    long response_id = 0;
    while(true)
    {
        cout << "Send Message " << endl;

        PackagePtr package_new = PACKAGE_MANAGER->AllocateRtnDepth();

        auto p_rtn_depth = GET_NON_CONST_FIELD(package_new, CUTRtnDepthField);
        package_new->prepare_response(UT_FID_RtnDepth, ++response_id);

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
    get_io_service().post(std::bind(&DataReceive::handle_depth_data, this, exchange, symbol, depth));
    return 1;
}

// K线数据（推送）
int DataReceive::on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines)
{
    // cout << "DataReceive::on_kline " << endl;

    get_io_service().post(std::bind(&DataReceive::handle_kline_data, this, exchange, symbol, resolution, klines));
    return 1;
}

void DataReceive::handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth)
{
    
    if (depth.symbol.length() == 0 || depth.symbol == "") 
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
    return ; 

    cout << "klines.size: " << klines.size() << endl;
    for( int i = 0 ; i < klines.size() ; i ++ )
    {
        const KlineData& kline = klines[i];

        std::stringstream stream_obj;
        stream_obj << "symbol: " << symbol << ", exchange: " << exchange << ", \n"
                    << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                    << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << "\n";

        LOG_INFO(stream_obj.str());

        PackagePtr package = GetNewKlineDataPackage(kline, ID_MANAGER->get_id());

        package->prepare_response(UT_FID_KlineData, ID_MANAGER->get_id());

        deliver_response(package);
    }

   
}

