#include "front_server.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "hub_struct.h"
#include "../util/tools.h"
#include "../front_server_declare.h"

FrontServer::FrontServer(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{
    wb_server_ = boost::make_shared<WBServer>();

    rest_server_ = boost::make_shared<RestServer>();
}

FrontServer::~FrontServer()
{

}

void FrontServer::launch() 
{
    cout << "FrontServer::launch " << endl;
    wb_server_->set_front_server(this);
    rest_server_->set_front_server(this);
    wb_server_->launch();
    rest_server_->launch();
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
    get_io_service().post(std::bind(&FrontServer::handle_response_message, this, package));
}

void FrontServer::handle_request_message(PackagePtr package)
{

}

void FrontServer::request_all_symbol()
{
    PackagePtr package = PackagePtr{new Package{}};
    package->prepare_request(UT_FID_SymbolData, ID_MANAGER->get_id());
    deliver_request(package);
}

void FrontServer::handle_response_message(PackagePtr package)
{
    // cout << "FrontServer::handle_response_message" << endl;

    switch (package->Tid())
    {
        case UT_FID_RtnDepth:
            process_rtn_depth_package(package);
            break;  

        case UT_FID_SDepthData:
            process_sdepth_package(package);
            break;

        case UT_FID_SymbolData:
            process_symbols_package(package);            
            break;

        case UT_FID_EnhancedDepthData:
            process_enhanceddata_package(package);            
            break;
        default:        
            cout << "Unknow Package" << endl;
            break;
    }    
}

void FrontServer::process_sdepth_package(PackagePtr package)
{
    cout << "FrontServer::process_sdepth_package " << endl;
    
    auto* psdepth = GET_FIELD(package, SDepthData);

    string send_str = SDepthDataToJsonStr(*psdepth);

    wb_server_->broadcast(send_str);
}

void FrontServer::process_rtn_depth_package(PackagePtr package)
{
    cout << "FrontServer::process_rtn_depth_package " << endl;
    
    auto prtn_depth = GET_FIELD(package, CUTRtnDepthField);

    printUTData(prtn_depth, UT_FID_RtnDepth);

    string send_str = convertUTData(prtn_depth, UT_FID_RtnDepth);

    wb_server_->broadcast(send_str);
}

void FrontServer::process_symbols_package(PackagePtr package)
{
    cout << "FrontServer::process_symbols_package " << endl;

    auto* p_symbol_data = GET_NON_CONST_FIELD(package, SymbolData);

    string updated_symbols_str = p_symbol_data->get_json_str();

    cout << "updated_symbols_str: " << updated_symbols_str << endl;

    symbols_.merge(p_symbol_data->get_symbols());

    wb_server_->broadcast(p_symbol_data->get_json_str());
}

void FrontServer::process_enhanceddata_package(PackagePtr package)
{
    cout << "FrontServer::process_enhanceddata_package " << endl;
    
    auto enhanced_data = GET_NON_CONST_FIELD(package, EnhancedDepthData);

    string send_str = enhanced_data->get_json_str();

    cout << "send_str: " << send_str << endl;

    wb_server_->broadcast(send_str);
}

string FrontServer::get_symbols_str()
{
    return SymbolsToJsonStr(symbols_);
}