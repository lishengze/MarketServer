#include "data_receive.h"
#include "../front_server_declare.h"
#include "quark/cxx/assign.h"
#include "pandora/util/time_util.h"
#include "pandora/package/package.h"
#include "../util/tools.h"

#include <chrono>


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

}

void DataReceive::handle_response_message(PackagePtr package)
{

}

// 深度数据（推送）
int DataReceive::on_depth(const char* exchange, const char* symbol, const SDepthData& depth)
{
    get_io_service().post(std::bind(&DataReceive::handle_depth_data, this, exchange, symbol, depth));
    return 0;
}

// K线数据（推送）
int DataReceive::on_kline(const char* exchange, const char* symbol, type_resolution resolution, const KlineData& kline)
{
    get_io_service().post(std::bind(&DataReceive::handle_kline_data, this, exchange, symbol, resolution, kline));
    return 1;
}

void DataReceive::handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth)
{
    cout << depth.exchange << " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << endl;

    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    package->prepare_response(UT_FID_SDepthData, package->PackageID());

    SDepthData* pDepthData = GET_NON_CONST_FIELD(package, SDepthData);

    cout << pDepthData->exchange << " " << pDepthData->symbol << " " << pDepthData->ask_length << " " << pDepthData->bid_length << endl;

    if (pDepthData->symbol.length() == 0 || pDepthData->symbol == "") return;

    // string json_str = SDepthDataToJsonStr(*pDepthData);

    // cout << "json_str: " << json_str << endl;

    deliver_response(package);
}

void DataReceive::handle_kline_data(const char* exchange, const char* symbol, type_resolution resolution, const KlineData& kline)
{

}

