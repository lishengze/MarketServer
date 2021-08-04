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
    // cout << "FrontServer::handle_response_message " << endl;

    switch (package->Tid())
    {
        case UT_FID_RspSymbolListData:
            response_symbol_list_package(package);            
            break;

        case UT_FID_RspRiskCtrledDepthData:
            response_depth_data_package(package);            
            break;          

        case UT_FID_RspKLineData:
            response_kline_data_package(package);
            break;

        case UT_FID_RspTrade:
            response_trade_data_package(package);
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

void FrontServer::response_symbol_list_package(PackagePtr package)
{
    try
    {
        RspSymbolListDataPtr p_symbol_list = GetField<RspSymbolListData>(package);

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
        RspRiskCtrledDepthDataPtr  pRspRiskCtrledDepthData = GetField<RspRiskCtrledDepthData>(package);

        string depth_str = pRspRiskCtrledDepthData->get_json_str();

        // cout <<"[Front Depth] socket_id: " << pRspRiskCtrledDepthData->socket_id_ << endl;
        // cout << depth_str << endl;

        

        if (!wb_server_->send_data(pRspRiskCtrledDepthData->socket_id_, depth_str))
        {
            cout << "FrontServer::response_depth_data_package Failed " << pRspRiskCtrledDepthData->socket_id_ << endl;

            string symbol = pRspRiskCtrledDepthData->depth_data_.symbol;

            PackagePtr req_cancel_pacakge = GetReqRiskCtrledDepthDataPackage(symbol, pRspRiskCtrledDepthData->socket_id_, ID_MANAGER->get_id(), true);

            if (req_cancel_pacakge)
            {
                deliver_request(req_cancel_pacakge);
            }
            else
            {
                std::stringstream stream_obj;
                stream_obj << "[E] FrontServer::response_depth_data_package: create cancel package Failed! \n";
                LOG_ERROR(stream_obj.str());                
            }
        }
        else
        {
            LOG->record_output_info("Depth_" + std::to_string(pRspRiskCtrledDepthData->socket_id_));
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
        // cout << "\nFrontServer::response_kline_data_package " << endl;
        
        RspKLineDataPtr p_rsp_kline_data = GetField<RspKLineData>(package);

        if (p_rsp_kline_data)
        {
            string kline_data_str = p_rsp_kline_data->get_json_str();

            // if (p_rsp_kline_data->frequency_ == 3600 && p_rsp_kline_data->is_update_ == true)
            // {
            //     return;
            // }

            string type = p_rsp_kline_data->is_update_ ? "_update_":"_init_";
            LOG->record_output_info("Kline_" + std::to_string(p_rsp_kline_data->socket_id_) 
                                    + "_fre_" + std::to_string(p_rsp_kline_data->frequency_)
                                    + type,
                                    p_rsp_kline_data->kline_data_vec_);  

                      

            // cout << "kline_data_str: " << kline_data_str << endl;


            // for (KlineDataPtr& atom_kline:p_rsp_kline_data->kline_data_vec_)
            // {
            //     cout << "[S] " << p_rsp_kline_data->socket_id_ << " "<< get_sec_time_str(atom_kline->index) << ", "
            //          << p_rsp_kline_data->symbol_ << "." << p_rsp_kline_data->frequency_ << ", "
            //          << "open: " << atom_kline->px_open.get_value() << ", "
            //          << "close: " << atom_kline->px_close.get_value() << ", "
            //          << "high: " << atom_kline->px_high.get_value() << ", "
            //          << "low: " << atom_kline->px_low.get_value() << endl;
            // }
            
            // cout << "[Front Kline]: Frequency: " << p_rsp_kline_data->frequency_ << " "
            //      << "data_size: " << p_rsp_kline_data->kline_data_vec_.size() << " "
            //      << "socket_id: " << p_rsp_kline_data->socket_id_ << endl;            

            // cout << "[Front Kline] socket_id: " << p_rsp_kline_data->socket_id_ << endl;

            // cout << kline_data_str << endl;

            if ((p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSOCKET || p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSECKETS))
            {
                if (!wb_server_->send_data(p_rsp_kline_data->socket_id_, kline_data_str))
                {
                    // cout << "Request Delete KLine Connect" << endl;

                    bool is_cancel_request = true;
                    PackagePtr package = CreatePackage<ReqKLineData>(p_rsp_kline_data->symbol_, p_rsp_kline_data->start_time_, p_rsp_kline_data->end_time_, 
                                                                        p_rsp_kline_data->data_count_, p_rsp_kline_data->frequency_, 
                                                                        p_rsp_kline_data->socket_id_, p_rsp_kline_data->socket_type_, is_cancel_request);
                    if(package)
                    {   
                        package->prepare_request(UT_FID_ReqKLineData, ID_MANAGER->get_id());

                        deliver_request(package);
                    }
                    else
                    {
                        LOG_ERROR("FrontServer::response_kline_data_package CreatePackage<ReqKLineData> Failed!");
                    } 
                }
                else
                {
                    // string type = p_rsp_kline_data->is_update_ ? "_update_":"_init_";
                    // LOG->record_output_info("Kline_" + std::to_string(p_rsp_kline_data->socket_id_) 
                    //                         + "_fre_" + std::to_string(p_rsp_kline_data->frequency_)
                    //                         + type,
                    //                         p_rsp_kline_data->kline_data_vec_);
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

void FrontServer::response_trade_data_package(PackagePtr package)
{
    try
    {
        // cout << "\nFrontServer::response_trade_data_package " << endl;
        
        RspTradePtr pRspTradeData = GetField<RspTrade>(package);

        if (pRspTradeData)
        {
            string trade_data_str = pRspTradeData->get_json_str();

            // cout << "[Front Trade] socket_id: " << pRspTradeData->socket_id_ << endl;  

            // cout << "trade_data_strl: "  << trade_data_str << endl;

            // cout << "[Front Trade] symbol: " << pRspTradeData->symbol_ << " "
            //      << "price: " << pRspTradeData->price_.get_value() << " "
            //      << "volume: " << pRspTradeData->volume_.get_value() << " "
            //      << "price: " << pRspTradeData->price_.get_value() << " \n"
            //      << "change_: " << pRspTradeData->change_ << " "
            //      << "change_rate_: " << pRspTradeData->change_rate_ << " "
            //      << "high_: " << pRspTradeData->high_.get_value() << " "  
            //      << "low_: " << pRspTradeData->low_.get_value() << " "        
            //      << "socket_id: " <<      pRspTradeData->socket_id_ << ""                     
            //      << "\n" << endl;

            if ((pRspTradeData->socket_type_ == COMM_TYPE::WEBSOCKET || pRspTradeData->socket_type_ == COMM_TYPE::WEBSECKETS))
            {
                if (!wb_server_->send_data(pRspTradeData->socket_id_, trade_data_str))
                {
                    cout << "Request Delete Trade Connect" << endl;

                    bool is_cancel_request = true;
                    PackagePtr package = CreatePackage<ReqTrade>(pRspTradeData->symbol_, true,
                                                                pRspTradeData->socket_id_, pRspTradeData->socket_type_);
                    if(package)
                    {   
                        package->prepare_request(UT_FID_ReqTrade, ID_MANAGER->get_id());

                        deliver_request(package);
                    }
                    else
                    {
                        LOG_ERROR("FrontServer::response_kline_data_package CreatePackage<ReqKLineData> Failed!");
                    } 
                }
                else
                {
                    LOG->record_output_info("Trade_" + std::to_string(pRspTradeData->socket_id_));                    
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
        stream_obj << "[E] FrontServer::response_trade_data_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_trade_data_package: unknown exceptions!\n";
        LOG_ERROR(stream_obj.str());        
    }
    
}

void FrontServer::response_enquiry_data_package(PackagePtr package)
{
    try
    {
        cout << "FrontServer::response_enquiry_data_package " << endl;

        RspEnquiryPtr pRspEnquiry = GetField<RspEnquiry>(package);

        if (pRspEnquiry)
        {
            string json_str = pRspEnquiry->get_json_str();
            LOG_DEBUG(json_str);
        }
        else
        {
            LOG_ERROR("FrontServer::response_enquiry_data_package RspEnquiry is NULL!");
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

        RspErrorMsgPtr pRspEnquiry = GetField<RspErrorMsg>(package);

        if (pRspEnquiry)
        {
            string json_str = pRspEnquiry->get_json_str();

            LOG_DEBUG(json_str);

            if (pRspEnquiry->socket_type_ == COMM_TYPE::HTTP || pRspEnquiry->socket_type_ == COMM_TYPE::HTTPS)
            {
                // p_rsp_enquiry->http_response_->end(json_str);
            }

            if (pRspEnquiry->socket_type_ == COMM_TYPE::WEBSOCKET || pRspEnquiry->socket_type_ == COMM_TYPE::WEBSECKETS)
            {
                wb_server_->send_data(pRspEnquiry->socket_id_, json_str);
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