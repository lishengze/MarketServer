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
            response_symbol_list_package(package);            
            break;

        case UT_FID_RspRiskCtrledDepthData:
            response_depth_data_package(package);            
            break;          

        case UT_FID_RspKLineData:
            response_kline_data_package(package);
            break;

        case UT_FID_RspEnquiry:
            response_enquiry_data_package(package);
            break;

        case UT_FID_RspErrorMsg:
            response_errmsg_package(package);
            break;
        default:        
            cout << "FrontServer::handle_response_message Unknow Package" << endl;
            break;
    }    
}

void FrontServer::response_sdepth_package(PackagePtr package)
{
    // cout << "FrontServer::response_sdepth_package " << endl;
    
    auto* psdepth = GET_FIELD(package, SDepthData);

    string send_str = SDepthDataToJsonStr(*psdepth);

    // wb_server_->broadcast(send_str);
}

void FrontServer::process_rtn_depth_package(PackagePtr package)
{
    cout << "FrontServer::process_rtn_depth_package " << endl;
    
    auto prtn_depth = GET_FIELD(package, CUTRtnDepthField);

    printUTData(prtn_depth, UT_FID_RtnDepth);

    string send_str = convertUTData(prtn_depth, UT_FID_RtnDepth);

    // wb_server_->broadcast(send_str);
}

void FrontServer::response_symbol_list_package(PackagePtr package)
{
    try
    {
        RspSymbolListData* p_symbol_list = GET_NON_CONST_FIELD(package, RspSymbolListData);

        if (p_symbol_list)
        {
            string symbol_list_str = p_symbol_list->get_json_str();

            cout << "symbol_list_str: " << symbol_list_str << endl;

            if (!wb_server_->send_data(p_symbol_list->socket_id_, symbol_list_str))
            {
                cout << "FrontServer::response_symbol_list_package Failed " << p_symbol_list->socket_id_ << endl;

                PackagePtr req_cancel_pacakge = GetReqSymbolListDataPackage(p_symbol_list->socket_id_, p_symbol_list->socket_type_, ID_MANAGER->get_id(), true);

                if (req_cancel_pacakge)
                {
                    deliver_request(req_cancel_pacakge);
                }
            }
        }
        else
        {
            cout << "FrontServer::response_symbol_list_package p_symbol_list empty!" << endl;
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    // cout << "FrontServer::process_symbols_package 0" << endl;

    // cout << "FrontServer::process_symbols_package 1" << endl;

    // std::set<std::string>& symbols = p_symbol_data->get_symbols();

    // string updated_symbols_str = SymbolsToJsonStr(*p_symbol_data, SYMBOL_UPDATE);

    // // cout << "FrontServer::process_symbols_package 2" << endl;

    // // cout << "FrontServer::process_symbols_package 3" << endl;

    // symbols_.merge(p_symbol_data->get_symbols());

    // // cout << "\nCurrent FrontServer Symbol: " << endl;

    // // for (auto symbol: symbols_)
    // // {
    // //     cout << symbol << endl;
    // // }

    // // cout << "FrontServer::process_symbols_package 4" << endl;

    
}

void FrontServer::response_depth_data_package(PackagePtr package)
{
    try
    {        
        RspRiskCtrledDepthData*  pRspRiskCtrledDepthData = GET_NON_CONST_FIELD(package, RspRiskCtrledDepthData);

        string depth_str = pRspRiskCtrledDepthData->get_json_str();

        // string update_symbol = enhanced_data->depth_data_.symbol;

        // string send_str = RspRiskCtrledDepthDataToJsonStr(*enhanced_data, MARKET_DATA_UPDATE);    

        // LOG_INFO(update_symbol);
        // LOG_INFO(send_str);

        // wb_server_->broadcast_enhanced_data(update_symbol, send_str);

        if (!wb_server_->send_data(pRspRiskCtrledDepthData->socket_id_, depth_str))
        {
            cout << "FrontServer::response_depth_data_package Failed " << pRspRiskCtrledDepthData->socket_id_ << endl;

            string symbol = pRspRiskCtrledDepthData->depth_data_.symbol;

            PackagePtr req_cancel_pacakge = GetReqRiskCtrledDepthDataPackage(symbol, pRspRiskCtrledDepthData->socket_id_, ID_MANAGER->get_id(), true);

            if (req_cancel_pacakge)
            {
                deliver_request(req_cancel_pacakge);
            }
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_depth_data_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::store_ws: unkonwn exception! " << "\n";
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

void FrontServer::response_kline_data_package(PackagePtr package)
{
    try
    {
        cout << "\nFrontServer::response_kline_data_package " << endl;

        RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

        if (p_rsp_kline_data)
        {
            string kline_data_str = p_rsp_kline_data->get_json_str();

            // cout << "kline_data_str: " << kline_data_str << endl;

            cout << "Frequency: " << p_rsp_kline_data->frequency_ << ", data_size: " << p_rsp_kline_data->kline_data_vec_.size() << endl;
            for (KlineDataPtr& atom_kline:p_rsp_kline_data->kline_data_vec_)
            {
                cout << get_sec_time_str(atom_kline->index) << ", "
                     << p_rsp_kline_data->symbol_ << ", "
                     << "open: " << atom_kline->px_open.get_value() << ", "
                     << "close: " << atom_kline->px_close.get_value() << ", "
                     << "high: " << atom_kline->px_high.get_value() << ", "
                     << "low: " << atom_kline->px_low.get_value() << endl;
            }
            cout << endl;

            // cout << kline_data_str << endl;

            if ((p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSOCKET || p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSECKETS))
            {
                if (!wb_server_->send_data(p_rsp_kline_data->socket_id_, kline_data_str))
                {
                    cout << "Request Delete KLine Connect" << endl;

                    PackagePtr package = PackagePtr{new Package{}};
            
                    package->SetPackageID(ID_MANAGER->get_id());

                    package->prepare_request(UT_FID_ReqKLineData, package->PackageID());

                    CREATE_FIELD(package, ReqKLineData);

                    ReqKLineData* p_req_kline_data = GET_NON_CONST_FIELD(package, ReqKLineData);

                    if (p_req_kline_data)
                    {
                        p_req_kline_data->set(p_rsp_kline_data->symbol_, p_rsp_kline_data->start_time_, p_rsp_kline_data->end_time_, 
                                              p_rsp_kline_data->data_count_, p_rsp_kline_data->frequency_, 
                                              p_rsp_kline_data->socket_id_, p_rsp_kline_data->socket_type_, true);  

                        deliver_request(package);
                    }    

                }
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

void FrontServer::response_enquiry_data_package(PackagePtr package)
{
    try
    {
        cout << "FrontServer::response_enquiry_data_package " << endl;

        RspEnquiry* p_rsp_enquiry = GET_NON_CONST_FIELD(package, RspEnquiry);

        if (p_rsp_enquiry)
        {
            string json_str = p_rsp_enquiry->get_json_str();

            // string json_str = "test";

            LOG_DEBUG(json_str);
        }
        else
        {
            LOG_ERROR("FrontServer::response_enquiry_data_package RspKLineData is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_enquiry_data_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void FrontServer::response_errmsg_package(PackagePtr package)
{
    try
    {
        cout << "FrontServer::response_enquiry_data_package " << endl;

        RspErrorMsg* p_rsp_enquiry = GET_NON_CONST_FIELD(package, RspErrorMsg);

        if (p_rsp_enquiry)
        {
            string json_str = p_rsp_enquiry->get_json_str();

            LOG_DEBUG(json_str);

            if (p_rsp_enquiry->socket_type_ == COMM_TYPE::HTTP || p_rsp_enquiry->socket_type_ == COMM_TYPE::HTTPS)
            {
                // p_rsp_enquiry->http_response_->end(json_str);
            }

            if (p_rsp_enquiry->socket_type_ == COMM_TYPE::WEBSOCKET || p_rsp_enquiry->socket_type_ == COMM_TYPE::WEBSECKETS)
            {
                wb_server_->send_data(p_rsp_enquiry->socket_id_, json_str);
            }
        }
        else
        {
            LOG_ERROR("FrontServer::response_errmsg_package RspKLineData is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_errmsg_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}