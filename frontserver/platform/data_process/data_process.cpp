#include "data_process.h"
#include "hub_interface.h"
#include "data_struct.h"
#include "../util/tools.h"
#include "../front_server_declare.h"

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
    get_io_service().post(std::bind(&DataProcess::handle_response_message, this, package));
}

void DataProcess::handle_request_message(PackagePtr package)
{
    switch (package->Tid())
    {
    case UT_FID_SymbolData:
        request_symbol_data();
        return;    
    default:
        break;
    }
    deliver_request(package);
}

void DataProcess::request_symbol_data()
{
    std::set<string> symbols;
    for (auto iter:depth_data_)
    {
        symbols.emplace(iter.first);
    }

    PackagePtr package_new = GetNewSymbolDataPackage(symbols, 0);
    package_new->prepare_response(UT_FID_SymbolData, package_new->PackageID());
    deliver_response(package_new);
}

void DataProcess::handle_response_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_SDepthData:
            process_sdepth_package(package);
            return;
        default:
            cout << "Unknow Package" << endl;
            break;
    }    

    deliver_response(package);
}

void DataProcess::process_sdepth_package(PackagePtr package)
{
    try
    {
        cout << "DataProcess::process_sdepth_package 0" << endl;
        SDepthData* p_depth_data = GET_NON_CONST_FIELD(package, SDepthData);

        cout << "DataProcess::process_sdepth_package 1" << endl;

        if (p_depth_data)
        {
            PackagePtr package_new = GetNewEnhancedDepthDataPackage(*p_depth_data, package->PackageID());

            cout << "DataProcess::process_sdepth_package 2" << endl;
            EnhancedDepthData* en_depth_data = GET_NON_CONST_FIELD(package_new, EnhancedDepthData);

            cout << "DataProcess::process_sdepth_package 3" << endl;
            if (depth_data_.find(en_depth_data->depth_data_.symbol) == depth_data_.end())
            {                
                cout << "DataProcess::process_sdepth_package 4.1" << endl;
                process_new_symbol(en_depth_data->depth_data_.symbol);
                cout << "DataProcess::process_sdepth_package 4.2" << endl;
            }

            depth_data_[en_depth_data->depth_data_.symbol] = en_depth_data->get_object();

            cout << "DataProcess::process_sdepth_package 5" << endl;

            deliver_response(package_new);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DataProcess::process_sdepth_package: " << e.what() << '\n';
    }
}

void DataProcess::process_new_symbol(string symbol)
{
    std::set<string> symbols{symbol};
    PackagePtr package_new = GetNewSymbolDataPackage(symbols, ID_MANAGER->get_id());
    package_new->prepare_response(UT_FID_SymbolData, package_new->PackageID());
    deliver_response(package_new);
}