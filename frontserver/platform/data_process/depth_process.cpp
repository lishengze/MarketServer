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

void DepthProces::request_depth_package(PackagePtr package)
{
    try
    {
        ReqRiskCtrledDepthData* p_req = GET_NON_CONST_FIELD(package, ReqRiskCtrledDepthData);

        if (p_req)
        {
            string symbol = string(p_req->symbol_);

            if (depth_data_.find(symbol) != depth_data_.end())
            {
                std::lock_guard<std::mutex> lk(depth_data_mutex_);

                PackagePtr enhanced_data_package = depth_data_[symbol];

                process_engine_->deliver_response(enhanced_data_package);
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

void DepthProces::request_symbol_package(PackagePtr package)
{
    std::set<string> symbols;

    std::lock_guard<std::mutex> lk(depth_data_mutex_);
    for (auto iter:depth_data_)
    {
        symbols.emplace(iter.first);
    }

    PackagePtr package_new = GetNewRspSymbolListDataPackage(symbols, 0);
    package_new->prepare_response(UT_FID_RspSymbolListData, package_new->PackageID());
    process_engine_->deliver_response(package_new);
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
                std::lock_guard<std::mutex> lk(raw_depth_data_mutex_);
                SDepthDataPtr raw_depth_data = boost::make_shared<SDepthData>(*p_depth_data);
                raw_depth_data_[string(p_depth_data->symbol)] = raw_depth_data;
            }
            else
            {
                PackagePtr enhanced_data_package = GetNewRspRiskCtrledDepthDataPackage(*p_depth_data, package->PackageID());

                RspRiskCtrledDepthData* en_depth_data = GET_NON_CONST_FIELD(enhanced_data_package, RspRiskCtrledDepthData);
                
                if (en_depth_data)
                {
                    std::lock_guard<std::mutex> lk(depth_data_mutex_);

                    string cur_symbol = string(en_depth_data->depth_data_.symbol);

                    if (depth_data_.find(cur_symbol) == depth_data_.end())
                    {                
                        response_new_symbol(cur_symbol);
                    }

                    depth_data_[cur_symbol] = enhanced_data_package;                
                }

                process_engine_->deliver_response(enhanced_data_package);
            }
            

        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DepthProces::response_src_sdepth_package: " << e.what() << '\n';
    }
}

void DepthProces::response_new_symbol(string symbol)
{
    // cout << "DepthProces::response_new_symbol 0" << endl;
    std::set<string> symbols{symbol};

    // cout << "DepthProces::response_new_symbol 1" << endl;
    PackagePtr package_new = GetNewRspSymbolListDataPackage(symbols, ID_MANAGER->get_id());

    // cout << "DepthProces::response_new_symbol 2" << endl;
    package_new->prepare_response(UT_FID_RspSymbolListData, package_new->PackageID());
    process_engine_->deliver_response(package_new);
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
                    PackagePtr  rsp_package = GetRspEnquiryPackage(symbol, price, p_req_enquiry->http_response_);
                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR(err_msg);
                    PackagePtr err_package = GetRspErrMsgPackage(err_msg, err_id, p_req_enquiry->http_response_);
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