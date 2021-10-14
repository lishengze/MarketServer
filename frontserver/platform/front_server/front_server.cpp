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
    LOG_INFO("FrontServer::launch ");
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
            LOG_WARN("FrontServer::handle_response_message Unknow Package");
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

            LOG->record_output_info("SymbolLists_" + std::to_string(p_symbol_list->socket_id_));
            
            if (!wb_server_->send_data(p_symbol_list->socket_id_, symbol_list_str))
            {

                PackagePtr req_cancel_pacakge = GetReqSymbolListDataPackage(p_symbol_list->socket_id_, p_symbol_list->socket_type_, ID_MANAGER->get_id(), true);

                if (req_cancel_pacakge)
                {
                    deliver_request(req_cancel_pacakge);
                }
            }
            else
            {
                LOG_WARN("FrontServer::response_symbol_list_package wb_server_->send_data Failed!");
            }

        }
        else
        {
            LOG_WARN("FrontServer::response_symbol_list_package response data null!");
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_symbol_list_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void FrontServer::response_depth_data_package(PackagePtr package)
{
    try
    {        
        RspRiskCtrledDepthDataPtr  pRspRiskCtrledDepthData = GetField<RspRiskCtrledDepthData>(package);

        if(pRspRiskCtrledDepthData)
        {
            string depth_str = pRspRiskCtrledDepthData->get_json_str();
            
            if (!wb_server_->send_data(pRspRiskCtrledDepthData->socket_id_, depth_str))
            {
                LOG_WARN("FrontServer::response_depth_data_package wb_server_->send_data Failed! Request Delete Connect!");

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
        else
        {
            LOG_WARN("FrontServer::response_depth_data_package response data null!");
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
        RspKLineDataPtr p_rsp_kline_data = GetField<RspKLineData>(package);

        if (p_rsp_kline_data)
        {
            string kline_data_str = p_rsp_kline_data->get_json_str();

            string type = p_rsp_kline_data->is_update_ ? "_update_":"_init_";
            LOG->record_output_info("Kline_" + std::to_string(p_rsp_kline_data->socket_id_) 
                                    + "_" + p_rsp_kline_data->symbol_ + "_"
                                    + "_fre_" + std::to_string(p_rsp_kline_data->frequency_)
                                    + type,
                                    p_rsp_kline_data->kline_data_vec_);  
            
            if (p_rsp_kline_data->is_update_)
            {
                string symbol = p_rsp_kline_data->symbol_;
                int frequency = p_rsp_kline_data->frequency_;
                if (sub_kline_map_.find(symbol) != sub_kline_map_.end() 
                    && sub_kline_map_[symbol].find(frequency) != sub_kline_map_[symbol].end())
                {
                    std::map<ID_TYPE, ReqKLineDataPtr>& sub_kline_map = sub_kline_map_[symbol][frequency];

                    std::vector<ID_TYPE> invalid_req_socket_vec;

                    for (auto iter: sub_kline_map)
                    {
                        if (!wb_server_->send_data(iter.first, kline_data_str))
                        {
                            LOG_WARN(string("wb_server_->send_data Failed! Invalid SocetID: ") + std::to_string(iter.first));
                            invalid_req_socket_vec.push_back(iter.first);
                        }
                    }

                    for (auto socket_id:invalid_req_socket_vec)
                    {
                        sub_kline_map.erase(socket_id);
                    }

                    if (sub_kline_map.size() == 0)
                    {
                        bool is_cancel_request = true;
                        PackagePtr package = CreatePackage<ReqKLineData>(p_rsp_kline_data->symbol_, 
                                                                         p_rsp_kline_data->start_time_, p_rsp_kline_data->end_time_, 
                                                                         p_rsp_kline_data->data_count_, p_rsp_kline_data->frequency_, 
                                                                         p_rsp_kline_data->socket_id_,  p_rsp_kline_data->socket_type_, 
                                                                         is_cancel_request);
                        if(package)
                        {   
                            package->prepare_request(UT_FID_ReqKLineData, ID_MANAGER->get_id());
                            deliver_request(package);
                        }
                        else
                        {
                            LOG_ERROR("CreatePackage<ReqKLineData> Failed!");
                        }                         
                    }
                }
            }
            else 
            {
                if ((p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSOCKET || p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSECKETS))
                {
                    if (!wb_server_->send_data(p_rsp_kline_data->socket_id_, kline_data_str))
                    {
                        LOG_WARN("wb_server_->send_data Failed! Request Delete Connect!");

                        // bool is_cancel_request = true;
                        // PackagePtr package = CreatePackage<ReqKLineData>(p_rsp_kline_data->symbol_, p_rsp_kline_data->start_time_, p_rsp_kline_data->end_time_, 
                        //                                                     p_rsp_kline_data->data_count_, p_rsp_kline_data->frequency_, 
                        //                                                     p_rsp_kline_data->socket_id_, p_rsp_kline_data->socket_type_, is_cancel_request);
                        // if(package)
                        // {   
                        //     package->prepare_request(UT_FID_ReqKLineData, ID_MANAGER->get_id());

                        //     deliver_request(package);
                        // }
                        // else
                        // {
                        //     LOG_ERROR("FrontServer::response_kline_data_package CreatePackage<ReqKLineData> Failed!");
                        // } 
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

void FrontServer::response_trade_data_package(PackagePtr package)
{
    try
    {
        RspTradePtr pRspTradeData = GetField<RspTrade>(package);

        if (pRspTradeData)
        {
            string trade_data_str = pRspTradeData->get_json_str();

            if ((pRspTradeData->socket_type_ == COMM_TYPE::WEBSOCKET || pRspTradeData->socket_type_ == COMM_TYPE::WEBSECKETS))
            {
                if (!wb_server_->send_data(pRspTradeData->socket_id_, trade_data_str))
                {
                    LOG_WARN("FrontServer::response_trade_data_package wb_server_->send_data Failed! Request Delete Connect!");

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
                        LOG_ERROR("FrontServer::response_trade_data_package CreatePackage<ReqKLineData> Failed!");
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
            LOG_ERROR("FrontServer::response_trade_data_package RspKLineData is NULL!");
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

void FrontServer::add_sub_kline(ReqKLineDataPtr req_kline_data)
{
    try
    {
        // remove current socket last sub req;
        if (sub_kline_socket_map_.find(req_kline_data->socket_id_) != sub_kline_socket_map_.end())
        {
            ReqKLineDataPtr& last_req_kline =  sub_kline_socket_map_[req_kline_data->socket_id_];

            LOG_INFO("Last ReqKlineInfo: " + req_kline_data->str());

            if (sub_kline_map_.find(last_req_kline->symbol_) != sub_kline_map_.end() 
            && sub_kline_map_[last_req_kline->symbol_].find(last_req_kline->frequency_) != sub_kline_map_[last_req_kline->symbol_].end()
            && sub_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].find(last_req_kline->socket_id_) 
                != sub_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].end())
            {
                LOG_INFO("sub_kline_map_ Erase Last ReqKline " + req_kline_data->simple_str());
                sub_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].erase(last_req_kline->socket_id_); 
            }

            sub_kline_socket_map_.erase(req_kline_data->socket_id_);
        }

        // add new sub req for current socket;
        // if (sub_kline_map_.find(req_kline_data->symbol_) == sub_kline_map_.end()
        //  || sub_kline_map_[req_kline_data->symbol_].find(req_kline_data->frequency_) 
        //     == sub_kline_map_[req_kline_data->symbol_].end())
        // {
        //     std::vector<ReqKLineDataPtr> empty_req_line_vec;
        //     sub_kline_map_[req_kline_data->symbol_][req_kline_data->frequency_] = empty_req_line_vec;
        // }

        LOG_INFO("New ReqKlineInfo: " + req_kline_data->str());
        sub_kline_socket_map_[req_kline_data->socket_id_] = req_kline_data;
        sub_kline_map_[req_kline_data->symbol_][req_kline_data->frequency_][req_kline_data->socket_id_] = req_kline_data;        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}