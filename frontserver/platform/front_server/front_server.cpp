#include "front_server.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "pandora/util/json.hpp"
#include "hub_struct.h"
#include "../util/tools.h"
#include "../front_server_declare.h"
#include "../log/log.h"

FrontServer::FrontServer(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{
    wb_server_ = boost::make_shared<WBServer>();
    
    rest_server_ = boost::make_shared<RestServer>(pool);
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

    // handle_response_message(package);
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
    cout << "FrontServer::handle_response_message" << endl;

    switch (package->Tid())
    {
        case UT_FID_RtnDepth:
            process_rtn_depth_package(package);
            break;  

        case UT_FID_SDepthData:
            response_sdepth_package(package);
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

void FrontServer::response_sdepth_package(PackagePtr package)
{
    cout << "FrontServer::response_sdepth_package " << endl;
    
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
    // cout << "FrontServer::process_symbols_package 0" << endl;

    SymbolData* p_symbol_data = GET_NON_CONST_FIELD(package, SymbolData);

    // cout << "FrontServer::process_symbols_package 1" << endl;

    std::set<std::string>& symbols = p_symbol_data->get_symbols();

    string updated_symbols_str = SymbolsToJsonStr(*p_symbol_data, SYMBOL_UPDATE);

    // cout << "FrontServer::process_symbols_package 2" << endl;

    cout << "updated_symbols_str: " << updated_symbols_str << endl;

    // cout << "FrontServer::process_symbols_package 3" << endl;

    symbols_.merge(p_symbol_data->get_symbols());

    // cout << "FrontServer::process_symbols_package 4" << endl;

    wb_server_->broadcast(updated_symbols_str);
}

void FrontServer::process_enhanceddata_package(PackagePtr package)
{
    // cout << "FrontServer::process_enhanceddata_package " << endl;
    
    auto enhanced_data = GET_NON_CONST_FIELD(package, EnhancedDepthData);

    wb_server_->broadcast_enhanced_data(*enhanced_data);
}

string FrontServer::get_symbols_str()
{
    return SymbolsToJsonStr(symbols_, SYMBOL_LIST);
}

string FrontServer::get_heartbeat_str()
{
    nlohmann::json json_obj;
    json_obj["type"] = "heartbeat";
    return json_obj.dump();
}

bool FrontServer::request_kline_data(string symbol, type_tick start_time_secs, type_tick end_time_secs, int frequency,
                                     HttpResponse* http_res, HttpRequest* http_req)
{
    try
    {
        cout << "FrontServer::request_kline_data " << endl;
        

        PackagePtr package = PackagePtr{new Package{}};
  
        package->SetPackageID(ID_MANAGER->get_id());

        package->prepare_request(UT_FID_ReqKLineData, package->PackageID());

        CREATE_FIELD(package, ReqKLineData);

        

        ReqKLineData* p_req_kline_data = GET_NON_CONST_FIELD(package, ReqKLineData);

        if (p_req_kline_data)
        {
            p_req_kline_data->comm_type = COMM_TYPE::HTTP;
            p_req_kline_data->http_request_ = http_req;
            p_req_kline_data->http_response_ = http_res;
            p_req_kline_data->symbol_ = symbol;
            p_req_kline_data->start_time_ = start_time_secs;
            p_req_kline_data->end_time_ = end_time_secs;
            p_req_kline_data->frequency_ = frequency;

            deliver_request(package);
        }
        else
        {
            return false;
            LOG_ERROR("FrontServer::request_kline_data Create ReqKLineData Failed!");
        }        
        return true;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::request_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());

        return false;
    }
    
}

void FrontServer::response_kline_data_package(PackagePtr package)
{
    try
    {
        cout << "FrontServer::response_kline_data_package " << endl;

        RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

        if (p_rsp_kline_data)
        {
            string response_data = RspKlinDataToJsonStr(*p_rsp_kline_data);
            cout << "response_data: " << response_data << endl;
            // if (p_rsp_kline_data->http_response_)
            // {
            //     p_rsp_kline_data->http_response_->end(response_data);
            // }
        }
        else
        {
            LOG_ERROR("FrontServer::response_kline_data_package RspKLineData is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_kline_data_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}
