#include "data_process.h"

DataProcess::DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{

}

DataProcess::~DataProcess()
{

}

void DataProcess::launch()
{
    cout << "DataProcess::launch " << endl;
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
    // cout << "DataProcess::response_message " << endl;
    get_io_service().post(std::bind(&DataProcess::handle_response_message, this, package));
}

void DataProcess::handle_request_message(PackagePtr package)
{
    deliver_request(package);
}

void DataProcess::handle_response_message(PackagePtr package)
{
    // cout << "DataProcess::handle_response_message " << endl;
    deliver_response(package);
}