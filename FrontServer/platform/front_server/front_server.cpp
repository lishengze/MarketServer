#include "front_server.h"

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

}

void FrontServer::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&FrontServer::handle_request_message, this, package));
}

void FrontServer::response_message(PackagePtr package)
{
    get_io_service().post(std::bind(&FrontServer::handle_response_message, this, package));
}

void FrontServer::handle_request_message(PackagePtr package)
{

}

void FrontServer::handle_response_message(PackagePtr package)
{

}