#include "../front_server_declare.h"
#include "../config/config.h"
#include "../log/log.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../util/id.hpp"

#include "depth_process.h"
#include "hub_interface.h"
#include "data_process.h"

DepthProces::DepthProces()
{

}

void DepthProces::init_process_engine(DataProcessPtr process_engine)
{
    process_engine_ = process_engine;
}

void DepthProces::request_symbol_list_package(PackagePtr package)
{
    try
    {
        ReqSymbolListDataPtr pReqSymbolListData = GetField<ReqSymbolListData>(package);

        std::set<string> symbols;

        if (pReqSymbolListData)
        {
            LOG_INFO(pReqSymbolListData->str());
            {
                std::lock_guard<std::mutex> lk(depth_data_mutex_);
                for (auto iter:depth_data_)
                {
                    symbols.emplace(iter.first);
                }
            }
            is_symbol_list_subed_ = true;

            // PackagePtr package_new = GetNewRspSymbolListDataPackage(symbols, pReqSymbolListData->socket_id_, 
            //                                                         pReqSymbolListData->socket_type_, 
            //                                                         ID_MANAGER->get_id());
            PackagePtr package = CreatePackage<RspSymbolListData>(symbols, pReqSymbolListData->websocket_);
            if (package)
            {
                package->prepare_response(UT_FID_RspSymbolListData, ID_MANAGER->get_id());
                process_engine_->deliver_response(package);
            }
            else
            {
                LOG_ERROR("CreatePackage<RspSymbolListData> Failed!");
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DepthProces::request_depth_package(PackagePtr package)
{
    try
    {
        ReqRiskCtrledDepthDataPtr p_req = GetField<ReqRiskCtrledDepthData>(package);

        if (p_req)
        {
            LOG_INFO("SubDepth: \n" +  p_req->str());
            if (p_req->is_canacel_request_)
            {
                delete_sub_depth_symbol(p_req->symbol_);
                return;
            }

            for (auto iter:depth_data_)
            {
                LOG_INFO("Depth-Symbol: " + iter.first);
            }
                        
            string symbol = string(p_req->symbol_);
            if (depth_data_.find(symbol) != depth_data_.end())
            {
                std::lock_guard<std::mutex> lk_d(depth_data_mutex_);
                SDepthDataPtr pSDepthData = depth_data_[symbol];

                std::stringstream stream_obj;
                stream_obj << "[SrcDepth] " << pSDepthData->exchange << " " << pSDepthData->symbol 
                           << " " << pSDepthData->ask_length << " " << pSDepthData->bid_length << "\n";                    
                LOG_INFO(stream_obj.str());
                
                // PackagePtr RspRiskCtrledDepthDataPackage = GetNewRspRiskCtrledDepthDataPackage(*pSDepthData, p_req->socket_id_, p_req->socket_type_, ID_MANAGER->get_id());           

                PackagePtr RspRiskCtrledDepthDataPackage = CreatePackage<RspRiskCtrledDepthData>(*pSDepthData, p_req->websocket_);
                if (RspRiskCtrledDepthDataPackage)
                {
                    RspRiskCtrledDepthDataPackage->prepare_response(UT_FID_RspRiskCtrledDepthData, ID_MANAGER->get_id()); 
                    std::lock_guard<std::mutex> lk_s(sub_depth_mutex_);
                    sub_depth_set_.emplace(symbol);

                    process_engine_->deliver_response(RspRiskCtrledDepthDataPackage);                    
                }    
                else
                {
                    LOG_ERROR("CreatePackage<RspRiskCtrledDepthData> Failed!");
                }
            }
            else
            {
                string err_msg = symbol + " does not have depth data";
                int error_id = -1;
                LOG_ERROR("err_msg: " + err_msg);
                PackagePtr err_package = GetRspErrMsgPackage(err_msg, error_id, p_req->socket_id_, p_req->socket_type_);
                if (err_package)
                {
                    process_engine_->deliver_response(err_package);
                }
                else
                {
                    LOG_ERROR("GetRspErrMsgPackage Failed!");
                }
            }
        }
        else 
        {
           LOG_DEBUG("req_data is null");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DepthProces::response_sdepth_package(PackagePtr package)
{
    try
    {
        SDepthDataPtr pSDepthData = GetField<SDepthData>(package);

        if (pSDepthData)
        {
            if (pSDepthData->is_raw)
            {
                std::lock_guard<std::mutex> lk(raw_depth_data_mutex_);
                SDepthDataPtr raw_depth_data = boost::make_shared<SDepthData>(*pSDepthData);
                raw_depth_data_[string(pSDepthData->symbol)] = raw_depth_data;
            }
            else
            {
                string cur_symbol = string(pSDepthData->symbol);
                std::lock_guard<std::mutex> lk(depth_data_mutex_);
               
                if (depth_data_.find(cur_symbol) == depth_data_.end())
                {     
                    LOG_INFO("New Symbol: " + cur_symbol);
                    response_new_symbol(cur_symbol);
                }

                depth_data_[cur_symbol] = pSDepthData;
                response_updated_depth_data(pSDepthData);
            }
        }
        else
        {
            LOG_ERROR("SDepthDataPtr is null");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DepthProces::response_updated_depth_data(SDepthDataPtr pSDepthData)
{
    try
    {
        string symbol = string(pSDepthData->symbol);

        std::lock_guard<std::mutex> lk_s(sub_depth_mutex_);
        if (sub_depth_set_.find(symbol) != sub_depth_set_.end())
        {
            PackagePtr package = CreatePackage<RspRiskCtrledDepthData>(*pSDepthData);
            if (package)
            {
                // std::stringstream stream_obj;
                // stream_obj  << "[Update] " << pSDepthData->exchange << " " << pSDepthData->symbol << " " << pSDepthData->ask_length << " " << pSDepthData->bid_length << "\n";                    
                // LOG_DEBUG(stream_obj.str());

                package->prepare_response(UT_FID_RspRiskCtrledDepthData, ID_MANAGER->get_id());
                process_engine_->deliver_response(package);
            }        
            else
            {
                LOG_ERROR("GetNewRspRiskCtrledDepthDataPackage Failed!");
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthProces::response_new_symbol(string symbol)
{
    try
    {
        if (is_symbol_list_subed_)
        {
            std::set<string> symbols{symbol};

            PackagePtr package_new = CreatePackage<RspSymbolListData>(symbols);
            if (package_new)
            {
                package_new->prepare_response(UT_FID_RspSymbolListData, ID_MANAGER->get_id());
                process_engine_->deliver_response(package_new);   
            }   
            else
            {
                LOG_ERROR("GetNewRspSymbolListDataPackage Failed!");
            }   
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DepthProces::request_enquiry_package(PackagePtr package)
{
    try
    {
        LOG_DEBUG("DepthProces::request_enquiry_package");

        ReqEnquiryPtr p_req_enquiry = GetField<ReqEnquiry>(package);

        if (p_req_enquiry)
        {
            double price = -1;
            string symbol = p_req_enquiry->symbol_;

            std::lock_guard<std::mutex> lk(raw_depth_data_mutex_);
            if (raw_depth_data_.find(symbol) != raw_depth_data_.end())
            {
                string err_msg;
                int err_id;
                SDepthDataPtr depth_data = raw_depth_data_[symbol];

                price = compute_enquiry_price(depth_data, p_req_enquiry->type_, p_req_enquiry->volume_, p_req_enquiry->amount_, err_msg, err_id);

                if (price != -1)
                {
                    PackagePtr  rsp_package = GetRspEnquiryPackage(symbol, price, p_req_enquiry->socket_id_, p_req_enquiry->socket_type_);
                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR(err_msg);
                    PackagePtr err_package = GetRspErrMsgPackage(err_msg, err_id, p_req_enquiry->socket_id_, p_req_enquiry->socket_type_);

                    if (err_package)
                    {
                        process_engine_->deliver_response(err_package);
                    }
                    else
                    {
                        LOG_ERROR("DepthProces::request_enquiry_package GetRspErrMsgPackage Failed!");
                    }
                   
                }
            }
            else
            {
                LOG_ERROR(string("\nRaw Depth data does not have symbol: ") + symbol );
            }
        }
        else
        {
            LOG_ERROR("DepthProces::request_enquiry_package data is null");
        }        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] DepthProces::request_enquiry_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

double DepthProces::compute_enquiry_price(SDepthDataPtr depth_data, int type, double volume, double amount, string& errr_msg, int& err_id)
{
    double price = -1;
    double origin_volume = volume;
    double origin_amount = amount;
    if (type == 0)
    {
        if (volume != -1)
        {
            double sum_amount = 0;
            int i;
            for (i = depth_data->ask_length-1; i >= 0 && volume > 0; --i)
            {
                if (volume - depth_data->asks[i].volume.get_value() >= 0)
                {
                    sum_amount += depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value();                    
                }
                else
                {
                    sum_amount += volume * depth_data->asks[i].price.get_value();
                }               

                volume -= depth_data->asks[i].volume.get_value(); 
            }
            if (i < 0)
            {
                double cur_sum_volume = origin_volume - volume;
                errr_msg = string("Depth Can't afford Enquiry Order! Volume: ") 
                         + std::to_string(origin_volume) + " is too big!"
                         + "Current Sum Volume is: " + std::to_string(cur_sum_volume);
                
                err_id = 1;
            }
            else
            {
                price = sum_amount / origin_volume;
            }            
        }
        else if (amount != -1)
        {
            double sum_volume = 0;
            int i;
            for (i = depth_data->ask_length-1; i >= 0 && amount > 0; --i)
            {
                if (amount - depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value() >= 0)
                {                    
                    sum_volume += depth_data->asks[i].volume.get_value();
                }
                else
                {
                    sum_volume += amount / depth_data->asks[i].price.get_value();
                }         

                amount -= depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value();       
            }

            if (i < 0)
            {
                double cur_sum_amount = origin_amount - amount;
                errr_msg = string("Depth Can't afford Enquiry Order! Amout: ") 
                         + std::to_string(cur_sum_amount) + " is too big!"
                         + "Current Sum Amount is: " + std::to_string(cur_sum_amount);
                err_id = 1;                
                return price;
            }
            else
            {
                price = origin_amount / sum_volume;
            }                        
        }
    }
    else if (type == 1)
    {
        SDepthLevelData* data = depth_data->bids;

        if (volume != -1)
        {
            double sum_amount = 0;
            int i;
            for (i = 0; i < depth_data->bid_length && volume > 0; ++i)
            {
                if (volume - depth_data->bids[i].volume.get_value() >= 0)
                {
                    sum_amount += depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value();
                    
                }
                else
                {
                    sum_amount += volume * depth_data->bids[i].price.get_value();
                }             
                volume -= depth_data->bids[i].volume.get_value();   
            }
            if (i ==  depth_data->bid_length)
            {
                double cur_sum_volume = origin_volume - volume;
                errr_msg = string("Depth Can't afford Enquiry Order! Volume: ") 
                         + std::to_string(origin_volume) + " is too big!"
                         + "Current Sum Volume is: " + std::to_string(cur_sum_volume);
                err_id = 1;                
                return price;
            }
            else
            {
                price = sum_amount / origin_volume;
            }                            
        }
        else if (amount != -1)
        {
            double sum_volume = 0;
            int i;
            for (i = 0; i < depth_data->bid_length && amount > 0; ++i)
            {
                if (amount - depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value() >= 0)
                {                    
                    sum_volume += depth_data->bids[i].volume.get_value();
                }
                else
                {
                    sum_volume += amount / depth_data->bids[i].price.get_value();
                }                
                amount -= depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value();
            }
            if (i ==  depth_data->bid_length)
            {
                double cur_sum_amount = origin_amount - amount;
                errr_msg = string("Depth Can't afford Enquiry Order! Amout: ") 
                         + std::to_string(cur_sum_amount) + " is too big!"
                         + "Current Sum Amount is: " + std::to_string(cur_sum_amount);
                err_id = 1;                
                return price;
            }
            else
            {
                price = origin_amount / sum_volume;
            }            
        }        
    }
    return price;
}

void DepthProces::delete_sub_depth_symbol(string symbol)
{
    try
    {
        LOG_INFO("delete_sub_depth_symbol " + symbol);
        std::lock_guard<std::mutex> lk_s(sub_depth_mutex_);

        if (sub_depth_set_.find(symbol) != sub_depth_set_.end())
        {
            sub_depth_set_.erase(symbol);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}