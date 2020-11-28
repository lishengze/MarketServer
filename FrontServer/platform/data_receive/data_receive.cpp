#include "data_receive.h"
#include "../front_server_declare.h"
#include "quark/cxx/assign.h"
#include "pandora/util/time_util.h"
#include "pandora/package/package.h"


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

    if (is_test_)
    {
        test_thread_ = std::make_shared<std::thread>(&DataReceive::test_main, this);
    }    
}

void DataReceive::test_main()
{
    long response_id = 0;
    while(true)
    {
        cout << "Send Message " << endl;

        PackagePtr package_new = PACKAGE_MANAGER.AllocateRtnDepth();

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