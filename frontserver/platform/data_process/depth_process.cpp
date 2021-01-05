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

}