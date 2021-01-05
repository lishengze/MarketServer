#include "front_server.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "pandora/util/json.hpp"
#include "hub_struct.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
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

    // handle_request_message(package);
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
    package->prepare_request(UT_FID_RspSymbolListData, ID_MANAGER->get_id());
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
            response_sdepth_package(package);
            break;

        case UT_FID_RspSymbolListData:
            process_symbols_package(package);            
            break;

        case UT_FID_RspRiskCtrledDepthData:
            process_enhanceddata_package(package);            
            break;                
        case UT_FID_RspKLineData:
            response_kline_data_package(package);

        case UT_FID_RspEnquiry:
            response_enquiry_data_package(package);

        default:        
            cout << "Unknow Package" << endl;
            break;
    }    
}

void FrontServer::response_sdepth_package(PackagePtr package)
{
    // cout << "FrontServer::response_sdepth_package " << endl;
    
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

    RspSymbolListData* p_symbol_data = GET_NON_CONST_FIELD(package, RspSymbolListData);

    // cout << "FrontServer::process_symbols_package 1" << endl;

    std::set<std::string>& symbols = p_symbol_data->get_symbols();


    string updated_symbols_str = SymbolsToJsonStr(*p_symbol_data, SYMBOL_UPDATE);

    // cout << "FrontServer::process_symbols_package 2" << endl;

    // cout << "FrontServer::process_symbols_package 3" << endl;

    symbols_.merge(p_symbol_data->get_symbols());

    // cout << "\nCurrent FrontServer Symbol: " << endl;

    // for (auto symbol: symbols_)
    // {
    //     cout << symbol << endl;
    // }

    // cout << "FrontServer::process_symbols_package 4" << endl;

    wb_server_->broadcast(updated_symbols_str);
}

void FrontServer::process_enhanceddata_package(PackagePtr package)
{
    try
    {        
        auto enhanced_data = GET_NON_CONST_FIELD(package, RspRiskCtrledDepthData);

        string update_symbol = enhanced_data->depth_data_.symbol;

        string send_str = RspRiskCtrledDepthDataToJsonStr(*enhanced_data, MARKET_DATA_UPDATE);    

        // LOG_INFO(update_symbol);
        // LOG_INFO(send_str);

        wb_server_->broadcast_enhanced_data(update_symbol, send_str);
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::process_enhanceddata_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
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

void FrontServer::request_kline_data(const ReqKLineData& req_kline)
{
    get_io_service().post(std::bind(&FrontServer::handle_request_kline_data, this, &req_kline));
}

void FrontServer::request_depth_data(const ReqRiskCtrledDepthData& req_depth)
{
    get_io_service().post(std::bind(&FrontServer::handle_request_depth_data, this, &req_depth));
}

void FrontServer::handle_request_kline_data(const ReqKLineData* req_kline)
{
    try
    {
        // cout << "FrontServer::request_kline_data 0" << endl;
        
        PackagePtr package = PackagePtr{new Package{}};
  
        package->SetPackageID(ID_MANAGER->get_id());

        package->prepare_request(UT_FID_ReqKLineData, package->PackageID());

        CREATE_FIELD(package, ReqKLineData);

        // cout << "FrontServer::request_kline_data 1" << endl;

        ReqKLineData* p_req_kline_data = GET_NON_CONST_FIELD(package, ReqKLineData);

        // cout << "FrontServer::request_kline_data 1.1" << endl;

        if (p_req_kline_data)
        {   
 
            // assign(p_req_kline_data->symbol_, req_kline->symbol_);
            
            // assign(p_req_kline_data->start_time_, req_kline->start_time_);
            
            // assign(p_req_kline_data->end_time_, req_kline->end_time_);
            
            // assign(p_req_kline_data->frequency_, req_kline->frequency_);

            // assign(p_req_kline_data->data_count_, req_kline->data_count_);
            
            // assign(p_req_kline_data->websocket_, req_kline->websocket_);
            
            // assign(p_req_kline_data->comm_type, req_kline->comm_type);

            // assign(p_req_kline_data->http_response_, req_kline->http_response_); 
   
            p_req_kline_data->reset(*req_kline);

            deliver_request(package);
        }
        else
        {
            return ;
            LOG_ERROR("FrontServer::request_kline_data Create ReqKLineData Failed!");
        }        
        return ;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::request_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());

        return;
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
            string kline_data_str = RspKlinDataToJsonStr(*p_rsp_kline_data);

            // cout << "kline_data_str: " << kline_data_str << endl;

            cout << "Frequency: " << p_rsp_kline_data->frequency_ << ", data_size: " << p_rsp_kline_data->kline_data_vec_.size() << endl;
            for (AtomKlineDataPtr& atom_kline:p_rsp_kline_data->kline_data_vec_)
            {
                cout << get_sec_time_str(atom_kline->tick_) << ", "
                     << p_rsp_kline_data->symbol_ << ", "
                     << "open: " << atom_kline->open_ << ", "
                     << "close: " << atom_kline->close_ << ", "
                     << "high: " << atom_kline->high_ << ", "
                     << "low: " << atom_kline->low_ << endl;
            }

            if ((p_rsp_kline_data->comm_type == COMM_TYPE::HTTP || p_rsp_kline_data->comm_type == COMM_TYPE::HTTPS) 
            && p_rsp_kline_data->http_response_)
            {
                p_rsp_kline_data->http_response_->end(kline_data_str);
            }

            if ((p_rsp_kline_data->comm_type == COMM_TYPE::WEBSOCKET || p_rsp_kline_data->comm_type == COMM_TYPE::WEBSECKETS) 
            && p_rsp_kline_data->websocket_)
            {
                wb_server_->send_data(p_rsp_kline_data->websocket_, kline_data_str);
            }

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

void FrontServer::handle_request_depth_data(const ReqRiskCtrledDepthData* req_depth)
{
    try
    {
        PackagePtr package = PackagePtr{new Package{}};
  
        package->SetPackageID(ID_MANAGER->get_id());

        package->prepare_request(UT_FID_ReqRiskCtrledDepthData, package->PackageID());

        CREATE_FIELD(package, ReqRiskCtrledDepthData);


        ReqRiskCtrledDepthData* p_req_depth_data = GET_NON_CONST_FIELD(package, ReqRiskCtrledDepthData);

        if (p_req_depth_data)
        {            
            assign(p_req_depth_data->symbol_, req_depth->symbol_);

            assign(p_req_depth_data->websocket_, req_depth->websocket_);
            assign(p_req_depth_data->comm_type, req_depth->comm_type);
            assign(p_req_depth_data->http_response_, req_depth->http_response_);

            deliver_request(package);
        }
        else
        {
            LOG_ERROR("FrontServer::request_depth_data Create ReqRiskCtrledDepthData Failed!");
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::handle_request_depth_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());

        return;
    }
    
}

void FrontServer::response_enquiry_data_package(PackagePtr package)
{
    try
    {
        cout << "FrontServer::response_enquiry_data_package " << endl;

        RspEnquiry* p_rsp_enquiry = GET_NON_CONST_FIELD(package, RspEnquiry);

        if (p_rsp_enquiry)
        {
            string json_str = p_rsp_enquiry->get_json_str();

            LOG_DEBUG(json_str);

            if ((p_rsp_enquiry->comm_type == COMM_TYPE::HTTP || p_rsp_enquiry->comm_type == COMM_TYPE::HTTPS) 
            && p_rsp_enquiry->http_response_)
            {
                p_rsp_enquiry->http_response_->end(json_str);
            }

            if ((p_rsp_enquiry->comm_type == COMM_TYPE::WEBSOCKET || p_rsp_enquiry->comm_type == COMM_TYPE::WEBSECKETS) 
            && p_rsp_enquiry->websocket_)
            {
                wb_server_->send_data(p_rsp_enquiry->websocket_, json_str);
            }
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