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
    cout << "DepthProces::request_symbol_list_package " << endl;

    std::set<string> symbols;

    ReqSymbolListData* pReqSymbolListData = GET_NON_CONST_FIELD(package, ReqSymbolListData);

    if (pReqSymbolListData)
    {
        if (pReqSymbolListData->is_canacel_request_)
        {
            delete_reqsymbollist_connection(pReqSymbolListData);
            return;
        }

        std::lock_guard<std::mutex> lk(depth_data_mutex_);
        for (auto iter:depth_data_)
        {
            symbols.emplace(iter.first);
            cout << "symbol: " << iter.first << endl;
        }

        PackagePtr package_new = GetNewRspSymbolListDataPackage(symbols, pReqSymbolListData->socket_id_, 
                                                                pReqSymbolListData->socket_type_, ID_MANAGER->get_id());

        process_engine_->deliver_response(package_new);

        {
            std::lock_guard<std::mutex> lk(req_symbol_list_map_mutex_);

            ReqSymbolListDataPtr req = boost::make_shared<ReqSymbolListData>(*pReqSymbolListData);

            req_symbol_list_map_[pReqSymbolListData->socket_id_] = req;
        }
    }
}


void DepthProces::request_depth_package(PackagePtr package)
{
    try
    {
        ReqRiskCtrledDepthData* p_req = GET_NON_CONST_FIELD(package, ReqRiskCtrledDepthData);

        if (p_req)
        {
            if (p_req->is_canacel_request_)
            {
                delete_subdepth_connection(p_req->symbol_, p_req->socket_id_);
                return;
            }
                        
            checkout_subdepth_connections(p_req);

            string symbol = string(p_req->symbol_);

            cout << "DepthProces::request_depth_package  " << symbol << endl;

            if (depth_data_.find(symbol) != depth_data_.end())
            {
                std::lock_guard<std::mutex> lk_d(depth_data_mutex_);

                SDepthDataPtr p_depth_data = depth_data_[symbol];

                PackagePtr RspRiskCtrledDepthDataPackage = GetNewRspRiskCtrledDepthDataPackage(*p_depth_data, p_req->socket_id_, p_req->socket_type_, ID_MANAGER->get_id());           

                if (RspRiskCtrledDepthDataPackage)
                {
                    process_engine_->deliver_response(RspRiskCtrledDepthDataPackage);
                }     

                std::lock_guard<std::mutex> lk_s(subdepth_map_mutex_);
                ReqRiskCtrledDepthDataPtr req_ptr = boost::make_shared<ReqRiskCtrledDepthData>(*p_req);
                subdepth_map_[string(p_req->symbol_)].push_back(req_ptr);
            }
            else
            {
                string err_msg = symbol + " does not have depth data";
                int error_id = -1;
                PackagePtr err_package = GetRspErrMsgPackage(err_msg, error_id, p_req->socket_id_, p_req->socket_type_);
                if (err_package)
                {
                    process_engine_->deliver_response(err_package);
                }
            }
        }
        else 
        {
            /* code */
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void DepthProces::response_src_sdepth_package(PackagePtr package)
{
    try
    {
        // cout << "DepthProces::response_src_sdepth_package 0" << endl;

        SDepthData* p_depth_data = GET_NON_CONST_FIELD(package, SDepthData);

        if (p_depth_data)
        {
            if (p_depth_data->is_raw)
            {
                cout << "response_src_sdepth_package " << p_depth_data->symbol << " is raw!" << endl;
                std::lock_guard<std::mutex> lk(raw_depth_data_mutex_);
                SDepthDataPtr raw_depth_data = boost::make_shared<SDepthData>(*p_depth_data);
                raw_depth_data_[string(p_depth_data->symbol)] = raw_depth_data;
            }
            else
            {
                // cout << "response_src_sdepth_package " << p_depth_data->symbol << endl;

                string cur_symbol = string(p_depth_data->symbol);

                SDepthDataPtr cur_depth_data = boost::make_shared<SDepthData>(*p_depth_data);

                std::lock_guard<std::mutex> lk(depth_data_mutex_);
               
                if (depth_data_.find(cur_symbol) == depth_data_.end())
                {                
                    cout << "New Symbol: " << cur_symbol << endl;

                    response_new_symbol(cur_symbol);
                }

                depth_data_[cur_symbol] = cur_depth_data;

                response_updated_depth_data(p_depth_data);
            }
        }
        else
        {
            cout << "p_depth_data is null" << endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DepthProces::response_src_sdepth_package: " << e.what() << '\n';
    }
}

void DepthProces::response_updated_depth_data(SDepthData* p_depth_data)
{
    try
    {
       std::lock_guard<std::mutex> lk(subdepth_map_mutex_);
       string symbol = string(p_depth_data->symbol);

        if (subdepth_map_.find(symbol) != subdepth_map_.end())
        {
            for (ReqRiskCtrledDepthDataPtr& iter:subdepth_map_[symbol])
            {
                PackagePtr package_new = GetNewRspRiskCtrledDepthDataPackage(*p_depth_data, iter->socket_id_, iter->socket_type_, ID_MANAGER->get_id());

                process_engine_->deliver_response(package_new);
            }
        }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void DepthProces::response_new_symbol(string symbol)
{
    // cout << "DepthProces::response_new_symbol 0" << endl;
    std::set<string> symbols{symbol};

    std::lock_guard<std::mutex> lk(req_symbol_list_map_mutex_);

    for (auto& iter:req_symbol_list_map_)
    {
        PackagePtr package_new = GetNewRspSymbolListDataPackage(symbols, iter.second->socket_id_, iter.second->socket_type_, ID_MANAGER->get_id());

        process_engine_->deliver_response(package_new);        
    }
}

void DepthProces::request_enquiry_package(PackagePtr package)
{
    try
    {
        LOG_DEBUG("DepthProces::request_enquiry_package");

        ReqEnquiry* p_req_enquiry = GET_NON_CONST_FIELD(package, ReqEnquiry);

        if (p_req_enquiry)
        {
            double price = -1;
            string symbol = p_req_enquiry->symbol_;

            // cout << "symbol: " << p_req_enquiry->symbol_ 
            //      << ", type: " << p_req_enquiry->type_ 
            //      << ", volume: " << p_req_enquiry->volume_ 
            //      << ", amount: " << p_req_enquiry->amount_ << endl;

            std::lock_guard<std::mutex> lk(raw_depth_data_mutex_);
            if (raw_depth_data_.find(symbol) != raw_depth_data_.end())
            {
                string err_msg;
                int err_id;
                SDepthDataPtr depth_data = raw_depth_data_[symbol];

                // cout << "Ask: length: " << depth_data->ask_length << endl;
                // for ( int i = 0; i < depth_data->ask_length; ++i)
                // {
                //     cout << depth_data->asks[i].price.get_value() << ", " << depth_data->asks[i].volume.get_value() << endl;
                // }

                // cout << "\nBid, length: " << depth_data->bid_length << endl;
                // for ( int i = 0; i < depth_data->bid_length; ++i)
                // {
                //     cout << depth_data->bids[i].price.get_value() << ", " << depth_data->bids[i].volume.get_value() << endl;
                // }    

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
                    process_engine_->deliver_response(err_package);
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

void DepthProces::delete_subdepth_connection(string symbol, ID_TYPE socket_id)
{
    cout << "DepthProces::delete_subdepth_connection " << symbol << " " << socket_id << endl;
    std::lock_guard<std::mutex> lk_s(subdepth_map_mutex_);
    if (subdepth_map_.find(symbol) != subdepth_map_.end())
    {
        vector<ReqRiskCtrledDepthDataPtr>::iterator iter = subdepth_map_[symbol].begin();
        while(iter != subdepth_map_[symbol].end())
        {
            if ((*iter)->socket_id_ == socket_id) break;
            ++iter;
        }

        if (iter != subdepth_map_[symbol].end())
        {
            cout << "DepthProces::delete_subdepth_connection Erase " << symbol <<" socket "<< socket_id << " S!" << endl;
            subdepth_map_[symbol].erase(iter);
        }
    }
}

void DepthProces::delete_reqsymbollist_connection(ReqSymbolListData* pReqSymbolListData)
{
    std::lock_guard<std::mutex> lk(req_symbol_list_map_mutex_);

    if (req_symbol_list_map_.find(pReqSymbolListData->socket_id_) != req_symbol_list_map_.end())
    {
        req_symbol_list_map_.erase(pReqSymbolListData->socket_id_);

        cout << "DepthProces::delete_reqsymbollist_connection delete socket: " << pReqSymbolListData->socket_id_ << " S!"<< endl;
    }
    else
    {
        cout << "DepthProces::delete_reqsymbollist_connection socket: " << pReqSymbolListData->socket_id_ << " does not exit!"<< endl;
    }
}

void DepthProces::checkout_subdepth_connections(ReqRiskCtrledDepthData* p_req)
{
    cout << "\nDepthProces::checkout_subdepth_connections " << p_req->symbol_ <<" " << p_req->socket_id_ << endl;
    std::lock_guard<std::mutex> lk(subdepth_con_map_mutex_);
    if (subdepth_con_map_.find(p_req->socket_id_) != subdepth_con_map_.end())
    {
        delete_subdepth_connection(subdepth_con_map_[p_req->socket_id_], p_req->socket_id_);        
    }

    subdepth_con_map_[p_req->socket_id_] = string(p_req->symbol_);   
}