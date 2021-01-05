#include "data_process.h"
#include "hub_interface.h"
#include "pandora/util/time_util.h"
#include "../data_structure/comm_data.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../front_server_declare.h"
#include "../log/log.h"
#include "../config/config.h"

DataProcess::DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{
    if (!kline_process_)
    {
        kline_process_ = boost::make_shared<KlineProcess>();
    }

    if (!depth_process_)
    {
        depth_process_ = boost::make_shared<DepthProces>();
    }
}

DataProcess::~DataProcess()
{

}

void DataProcess::launch()
{
    cout << "DataProcess::launch " << endl;
    if (kline_process_)
    {
        kline_process_->init_process_engine(shared_from_this());
    }

    if (depth_process_)
    {
        depth_process_->init_process_engine(shared_from_this());
    }    
}

void DataProcess::release()
{

}
            
void DataProcess::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataProcess::handle_request_message, this, package));
}

void DataProcess::response_message(PackagePtr package)
{
    handle_response_message(package);

    // get_io_service().post(std::bind(&DataProcess::handle_response_message, this, package));
}

void DataProcess::handle_request_message(PackagePtr package)
{
    cout << "DataProcess::handle_request_message: " << package->Tid() << endl;

    switch (package->Tid())
    {
        case UT_FID_RspSymbolListData:
            request_symbol_package(package);
            return;    

        case UT_FID_ReqRiskCtrledDepthData:
            request_depth_package(package);
            return;               

        case UT_FID_ReqKLineData:
            request_kline_package(package);

        case UT_FID_ReqEnquiry:
            request_enquiry_package(package);
            return;
        default:
            break;
    }
}

void DataProcess::handle_response_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_SDepthData:
            response_src_sdepth_package(package);
            return;

        case UT_FID_KlineData:
            response_src_kline_package(package);
            return;

        default:
            cout << "Unknow Package" << endl;
            break;
    }    

    deliver_response(package);
}

void DataProcess::request_enquiry_package(PackagePtr package)
{
    
}

void DataProcess::request_kline_package(PackagePtr package)
{
    try
    {
        if (kline_process_)
        {
            kline_process_->request_kline_package(package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void DataProcess::request_depth_package(PackagePtr package)
{
    try
    {
        if (depth_process_)
        {
            depth_process_->request_depth_package(package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void DataProcess::request_symbol_package(PackagePtr package)
{
    try
    {
        if (depth_process_)
        {
            depth_process_->request_symbol_package(package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void DataProcess::response_src_sdepth_package(PackagePtr package)
{
    try
    {
        if (depth_process_)
        {
            depth_process_->response_src_sdepth_package(package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void DataProcess::response_src_kline_package(PackagePtr package)
{
    try
    {
        if (kline_process_)
        {
            kline_process_->response_src_kline_package(package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}
