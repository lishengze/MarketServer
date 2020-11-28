#include "front_server.h"
#include "quark/cxx/ut/UtPrintUtils.h"

FrontServer::FrontServer(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{
    wb_server_ = boost::make_shared<WBServer>();
}

FrontServer::~FrontServer()
{

}

void FrontServer::launch() 
{
    cout << "FrontServer::launch " << endl;
    wb_server_->launch();
}

void FrontServer::release() 
{
    wb_server_->release();
}

void FrontServer::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&FrontServer::handle_request_message, this, package));
}

void FrontServer::response_message(PackagePtr package)
{
    // cout << "FrontServer::response_message " << endl;
    get_io_service().post(std::bind(&FrontServer::handle_response_message, this, package));
}

void FrontServer::handle_request_message(PackagePtr package)
{

}

void FrontServer::handle_response_message(PackagePtr package)
{
    // cout << "FrontServer::handle_response_message" << endl;

    switch (package->Tid())
    {
        case UT_FID_RtnDepth:
            process_rtn_depth_package(package);
            break;  
        
        default:
            cout << "Unknow Package" << endl;
            break;
    }    
}

void FrontServer::process_rtn_depth_package(PackagePtr package)
{
    cout << "FrontServer::process_rtn_depth_package " << endl;
    
    auto prtn_depth = GET_FIELD(package, CUTRtnDepthField);

    printUTData(prtn_depth, UT_FID_RtnDepth);

    string send_str = convertUTData(prtn_depth, UT_FID_RtnDepth);

    wb_server_->broadcast(send_str);
}