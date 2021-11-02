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
    // LOG_INFO("PackageInfo: " + get_package_str(package->Tid()));

    // get_io_service().post(std::bind(&FrontServer::handle_response_message, this, package));

    handle_response_message(package);
}

void FrontServer::handle_request_message(PackagePtr package)
{

}

void FrontServer::handle_response_message(PackagePtr package)
{
    try
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
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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

            std::set<ReqSymbolListDataPtr> invalid_sub_set;

            for(ReqSymbolListDataPtr req_symbol_list:sub_symbol_list_set_)
            {
                if (!req_symbol_list->websocket_)
                {
                    LOG_WARN("req_symbol_list->websocket_ is null");
                    invalid_sub_set.emplace(req_symbol_list);
                    continue;
                }

                if (!req_symbol_list->websocket_->is_alive())
                {
                    LOG_WARN("req_symbol_list->websocket_ " + req_symbol_list->websocket_->get_ws_str() + " is not alive!");
                    invalid_sub_set.emplace(req_symbol_list);
                    continue;                
                }

                p_symbol_list->websocket_->send(symbol_list_str);
            }

            for(ReqSymbolListDataPtr invalid_req:invalid_sub_set)
            {
                sub_symbol_list_set_.erase(invalid_req);
            }

        }
        else
        {
            LOG_WARN("response data null!");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
            string symbol = pRspRiskCtrledDepthData->depth_data_.symbol;

            std::lock_guard<std::mutex> lk(sub_depth_map_update_mutex_);
            // LOG_INFO(depth_str);
            if (sub_depth_map_.find(symbol) != sub_depth_map_.end())
            {
                map<ID_TYPE, ReqRiskCtrledDepthDataPtr>& cur_sub_depth_map = sub_depth_map_[symbol];
                std::vector<ID_TYPE> invalid_req_socket_vec;
                // LOG_INFO("cur_sub_depth_map.size: " + std::to_string(cur_sub_depth_map.size()));

                for (auto iter:cur_sub_depth_map)
                {
                    ReqRiskCtrledDepthDataPtr req_ptr = iter.second;
                    ID_TYPE socket_id = iter.first;

                    if (!req_ptr->websocket_)
                    {
                        LOG_ERROR("req_ptr->websocket_ is null");
                        invalid_req_socket_vec.push_back(socket_id);
                        continue;
                    }
                    if (!req_ptr->websocket_->is_alive())
                    {
                        LOG_ERROR("req_ptr->websocket_ "+ req_ptr->websocket_->get_ws_str() + " is not alive!");
                        wb_server_->close_ws(req_ptr->websocket_);
                        invalid_req_socket_vec.push_back(socket_id);
                        continue;                
                    }
                    req_ptr->websocket_->send(depth_str);
                }

                // LOG_INFO("invalid_req_socket_vec.size: " + std::to_string(invalid_req_socket_vec.size()));
                for (auto socket_id:invalid_req_socket_vec)
                {
                    cur_sub_depth_map.erase(socket_id);
                }

                // 取消对当前 Symbol 的 depth 订阅;
                if (cur_sub_depth_map.size() == 0)
                {
                    bool is_cancel_request = true;
                    PackagePtr package = GetReqRiskCtrledDepthDataPackage(symbol, pRspRiskCtrledDepthData->socket_id_, ID_MANAGER->get_id(), true);
                    if(package)
                    {
                        deliver_request(package);
                    }
                    else
                    {
                        LOG_ERROR("CreatePackage<ReqTrade> Failed!");
                    }                     
                }
            }
            else
            {
                LOG_WARN("sub_depth_map_ does not sub " + symbol);
            }
        }
        else
        {
            LOG_ERROR("pRspRiskCtrledDepthData is NULL!");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unkonwn exception! ");
    }
        
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

                std::lock_guard<std::mutex> lk(sub_kline_map_update_mutex_);
                if (sub_updated_kline_map_.find(symbol) != sub_updated_kline_map_.end() 
                    && sub_updated_kline_map_[symbol].find(frequency) != sub_updated_kline_map_[symbol].end())
                {
                    std::map<ID_TYPE, ReqKLineDataPtr>& sub_kline_map = sub_updated_kline_map_[symbol][frequency];
                    std::vector<ID_TYPE> invalid_req_socket_vec;

                    for (auto iter: sub_kline_map)
                    {
                        ReqKLineDataPtr req_ptr = iter.second;
                        ID_TYPE socket_id = iter.first;

                        if (!req_ptr->websocket_)
                        {
                            LOG_ERROR("req_ptr->websocket_ is null");
                            invalid_req_socket_vec.push_back(socket_id);
                            continue;
                        }
                        if (!req_ptr->websocket_->is_alive())
                        {
                            LOG_ERROR("req_ptr->websocket_ "+ req_ptr->websocket_->get_ws_str() + " is not alive!");
                            wb_server_->close_ws(req_ptr->websocket_);
                            invalid_req_socket_vec.push_back(socket_id);
                            continue;                
                        }
                        req_ptr->websocket_->send(kline_data_str);
                    }

                    for (auto socket_id:invalid_req_socket_vec)
                    {
                        sub_kline_map.erase(socket_id);
                    }

                    // 取消对当前 Symbol, Frequency 的 kline 订阅;
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
                else
                {
                    if (sub_updated_kline_map_.find(symbol) != sub_updated_kline_map_.end())
                    {
                        LOG_WARN("sub_updated_kline_map_ does not sub " + symbol);
                    }
                    else if (sub_updated_kline_map_[symbol].find(frequency) != sub_updated_kline_map_[symbol].end())
                    {
                        LOG_WARN("sub_updated_kline_map " + symbol + " does not sub fre: " + std::to_string(frequency));
                    }                    
                }
            }
            else 
            {
                if ((p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSOCKET || p_rsp_kline_data->socket_type_ == COMM_TYPE::WEBSECKETS))
                {
                    if (!p_rsp_kline_data->websocket_)
                    {
                        LOG_ERROR(" p_rsp_kline_data->websocket_ is null");
                        return;
                    }
                    if (!p_rsp_kline_data->websocket_->is_alive())
                    {
                        LOG_ERROR("p_rsp_kline_data->websocket_ "+ p_rsp_kline_data->websocket_->get_ws_str() + " is not alive!");
                        return;                
                    }
                    p_rsp_kline_data->websocket_->send(kline_data_str);
                }
            }
        }
        else
        {
            LOG_ERROR("pRspTradeData is NULL!");
        }        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
            std::lock_guard<std::mutex> lk(sub_trade_map_update_mutex_);

            if (sub_updated_trade_map_.find(pRspTradeData->symbol_) != sub_updated_trade_map_.end())
            {
                std::set<ReqTradePtr>& cur_sub_trade_set = sub_updated_trade_map_[pRspTradeData->symbol_];
                std::vector<ReqTradePtr> invalid_req_socket_vec;

                for (const ReqTradePtr& req_ptr:cur_sub_trade_set)
                {
                    if (!req_ptr->websocket_)
                    {
                        LOG_ERROR("req_ptr->websocket_ is null");
                        invalid_req_socket_vec.push_back(req_ptr);
                        continue;
                    }
                    if (!req_ptr->websocket_->is_alive())
                    {
                        LOG_ERROR("req_ptr->websocket_ "+ req_ptr->websocket_->get_ws_str() + " is not alive!");
                        wb_server_->close_ws(req_ptr->websocket_);
                        invalid_req_socket_vec.push_back(req_ptr);
                        continue;                
                    }
                    req_ptr->websocket_->send(trade_data_str);
                }

                for (auto req_ptr:invalid_req_socket_vec)
                {
                    cur_sub_trade_set.erase(req_ptr);
                }          

                // 取消对当前 Symbol 的 trade 订阅;
                if (cur_sub_trade_set.size() == 0)
                {
                    bool is_cancel_request = true;
                    PackagePtr package = CreatePackage<ReqTrade>(pRspTradeData->symbol_, is_cancel_request,
                                                                 pRspTradeData->socket_id_, pRspTradeData->socket_type_);
                    if(package)
                    {   
                        package->prepare_request(UT_FID_ReqTrade, ID_MANAGER->get_id());

                        deliver_request(package);
                    }
                    else
                    {
                        LOG_ERROR("CreatePackage<ReqTrade> Failed!");
                    }                     
                }                      

            }
        }
        else
        {
            LOG_ERROR("pRspTradeData is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unknown exceptions!");        
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

        RspErrorMsgPtr pRspError = GetField<RspErrorMsg>(package);

        if (pRspError)
        {
            string json_str = pRspError->get_json_str();

            LOG_WARN(json_str);

            if (!pRspError->websocket_)
            {
                LOG_ERROR(" pRspError->websocket_ is null");
                return;
            }
            if (!pRspError->websocket_->is_alive())
            {
                LOG_ERROR("pRspError->websocket_ "+ pRspError->websocket_->get_ws_str() + " is not alive!");
                return;                
            }
            pRspError->websocket_->send(json_str);

            // if (pRspEnquiry->socket_type_ == COMM_TYPE::HTTP || pRspEnquiry->socket_type_ == COMM_TYPE::HTTPS)
            // {
            //     // p_rsp_enquiry->http_response_->end(json_str);
            // }

            // if (pRspEnquiry->socket_type_ == COMM_TYPE::WEBSOCKET || pRspEnquiry->socket_type_ == COMM_TYPE::WEBSECKETS)
            // {
            //     wb_server_->send_data(pRspEnquiry->socket_id_, json_str);
            // }
        }
        else
        {
            LOG_ERROR("pRspError is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_errmsg_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void FrontServer::add_sub_symbol_list(ReqSymbolListDataPtr req_symbol_list)
{
    try
    {
        std::lock_guard<std::mutex> lk(sub_symbol_list_set_mutex_);

        if (sub_symbol_list_set_.find(req_symbol_list) == sub_symbol_list_set_.end())
        {
            sub_symbol_list_set_.emplace(req_symbol_list);
        }

        LOG_INFO("New ReqSymbolList: " + req_symbol_list->str());
    }
    catch(const std::exception& e)
    {
        LOG_WARN(e.what());
    }
}

void FrontServer::add_sub_kline(ReqKLineDataPtr req_kline_data)
{
    try
    {
        // remove current socket last sub req;
        std::lock_guard<std::mutex> lk(sub_kline_map_update_mutex_);
        if (sub_kline_socket_map_.find(req_kline_data->socket_id_) != sub_kline_socket_map_.end())
        {
            ReqKLineDataPtr last_req_kline =  sub_kline_socket_map_[req_kline_data->socket_id_];

            LOG_INFO("Last ReqKlineInfo: " + last_req_kline->str());

            if (sub_updated_kline_map_.find(last_req_kline->symbol_) != sub_updated_kline_map_.end() 
            && sub_updated_kline_map_[last_req_kline->symbol_].find(last_req_kline->frequency_) != sub_updated_kline_map_[last_req_kline->symbol_].end()
            && sub_updated_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].find(last_req_kline->socket_id_) 
                != sub_updated_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].end())
            {
                LOG_INFO("sub_updated_kline_map_ Erase Last ReqKline " + last_req_kline->simple_str());
                sub_updated_kline_map_[last_req_kline->symbol_][last_req_kline->frequency_].erase(last_req_kline->socket_id_); 
            }

            sub_kline_socket_map_.erase(req_kline_data->socket_id_);
        }

        LOG_INFO("New ReqKlineInfo: " + req_kline_data->str());
        sub_kline_socket_map_[req_kline_data->socket_id_] = req_kline_data;
        sub_updated_kline_map_[req_kline_data->symbol_][req_kline_data->frequency_][req_kline_data->socket_id_] = req_kline_data;        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void FrontServer::add_sub_trade(ReqTradePtr req_trade_data)
{
    try
    {
        // remove current socket last sub req;
        std::lock_guard<std::mutex> lk(sub_trade_map_update_mutex_);
        if (sub_trade_socket_map_.find(req_trade_data->socket_id_) != sub_trade_socket_map_.end())
        {
            ReqTradePtr last_req_trade = sub_trade_socket_map_[req_trade_data->socket_id_];

            LOG_INFO("Last ReqTradeInfo: " + last_req_trade->str());

            sub_trade_socket_map_.erase(last_req_trade->socket_id_);

            if (sub_updated_trade_map_.find(last_req_trade->symbol_) != sub_updated_trade_map_.end()
            && sub_updated_trade_map_[last_req_trade->symbol_].find(last_req_trade) != sub_updated_trade_map_[last_req_trade->symbol_].end())
            {
                LOG_INFO("sub_updated_trade_map_ erase last req_trade " + last_req_trade->str());

                sub_updated_trade_map_[last_req_trade->symbol_].erase(last_req_trade);
            }
        }

        LOG_INFO("New ReqTradeInfo: " + req_trade_data->str());
        sub_trade_socket_map_[req_trade_data->socket_id_] = req_trade_data;
        sub_updated_trade_map_[req_trade_data->symbol_].emplace(req_trade_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }        
}

void FrontServer::add_sub_depth(ReqRiskCtrledDepthDataPtr req_depth_data)
{
    try
    {
        // remove current socket last sub req;
        std::lock_guard<std::mutex> lk(sub_depth_map_update_mutex_);
        if (sub_depth_socket_map_.find(req_depth_data->socket_id_) != sub_depth_socket_map_.end())
        {
            ReqRiskCtrledDepthDataPtr last_req_depth =  sub_depth_socket_map_[req_depth_data->socket_id_];

            LOG_INFO("Last ReqDepthInfo: " + last_req_depth->str());
            sub_depth_socket_map_.erase(last_req_depth->socket_id_);

            if (sub_depth_map_.find(last_req_depth->symbol_) != sub_depth_map_.end()
             && sub_depth_map_[last_req_depth->symbol_].find(last_req_depth->socket_id_) 
             != sub_depth_map_[last_req_depth->symbol_].end())
            {
                LOG_INFO("sub_depth_map_ erase last req_depth " + last_req_depth->str());

                sub_depth_map_[last_req_depth->symbol_].erase(last_req_depth->socket_id_);
            }
        }

        LOG_INFO("New ReqDepthInfo: " + req_depth_data->str());
        sub_depth_socket_map_[req_depth_data->socket_id_] = req_depth_data;
        sub_depth_map_[req_depth_data->symbol_][req_depth_data->socket_id_] = req_depth_data;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }        
    catch(...)
    {
        LOG_ERROR("unknown exception!");
    }
}