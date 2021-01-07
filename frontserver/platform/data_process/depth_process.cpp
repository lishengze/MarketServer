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

            cout << "symbol: " << p_req_enquiry->symbol_ 
                 << ", type: " << p_req_enquiry->type_ 
                 << ", volume: " << p_req_enquiry->volume_ 
                 << ", amount: " << p_req_enquiry->amount_ << endl;

            SDepthDataPtr depth_data = raw_depth_data_[string(p_req_enquiry->symbol_ )];

            price = compute_enquiry_price(depth_data, p_req_enquiry->type_, p_req_enquiry->volume_, p_req_enquiry->amount_);

            if (price != -1)
            {
                PackagePtr  rsp_package = GetRspEnquiryPackage(p_req_enquiry->symbol_, price, p_req_enquiry->http_response_);
                process_engine_->deliver_response(rsp_package);
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

double DepthProces::compute_enquiry_price(SDepthDataPtr depth_data, int type, double volume, double amount)
{
    double price = -1;
    if (type == 0)
    {

        if (volume != -1)
        {
            double sum_amount = 0;
            for (int i = depth_data->ask_length; i >= 0; --i)
            {
                if (volume - depth_data->asks[i].volume.get_value() >= 0)
                {
                    sum_amount += depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value();
                    volume -= depth_data->asks[i].volume.get_value();
                }
                else
                {
                    sum_amount += volume * depth_data->asks[i].price.get_value();
                    break;
                }                
            }
            if (i < 0)
            {

                return price;
            }
            else
            {
                price = sum_amount / volume;
            }            
        }
        else if (amount != -1)
        {
            double sum_volume = 0;
            for (int i = depth_data->ask_length; i >= 0; --i)
            {
                if (amout - depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value() >= 0)
                {
                    amout -= depth_data->asks[i].volume.get_value() * depth_data->asks[i].price.get_value();
                    sum_volume += depth_data->asks[i].volume.get_value();
                }
                else
                {
                    sum_volume += amount / depth_data->asks[i].price.get_value();
                    break;
                }                
            }

            if (i < 0)
            {
                return price;
            }
            else
            {
                price = amount / sum_volume;
            }                        
        }
    }
    else if (type == 1)
    {
        SDepthLevelData* data = depth_data->bids;

        if (volume != -1)
        {
            double sum_amount = 0;
            for (int i = 0; i < depth_data->bid_length; ++i)
            {
                if (volume - depth_data->bids[i].volume.get_value() >= 0)
                {
                    sum_amount += depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value();
                    volume -= depth_data->bids[i].volume.get_value();
                }
                else
                {
                    sum_amount += volume * depth_data->bids[i].price.get_value();
                    break;
                }                
            }
            if (i ==  depth_data->bid_length)
            {
                return price;
            }
            else
            {
                price = sum_amount / volume;
            }                            
        }
        else if (amount != -1)
        {
            double sum_volume = 0;
            for (int i = 0; i < depth_data->bid_length; ++i)
            {
                if (amout - depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value() >= 0)
                {
                    amout -= depth_data->bids[i].volume.get_value() * depth_data->bids[i].price.get_value();
                    sum_volume += depth_data->bids[i].volume.get_value();
                }
                else
                {
                    sum_volume += amount / depth_data->bids[i].price.get_value();
                    break;
                }                
            }
            if (i ==  depth_data->bid_length)
            {
                return price;
            }
            else
            {
                price = amount / sum_volume;
            }            
        }        
    }
    return price;
}