#include "data_process.h"
#include "hub_interface.h"
#include "../data_structure/data_struct.h"
#include "../util/tools.h"
#include "../front_server_declare.h"
#include "../log/log.h"

DataProcess::DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{

}

DataProcess::~DataProcess()
{

}

void DataProcess::launch()
{
    cout << "DataProcess::launch " << endl;
}

void DataProcess::release()
{

}
            
void DataProcess::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataProcess::handle_request_message, this, package));
}

void DataProcess::response_message(PackagePtr package)
{
    // handle_response_message(package);

    get_io_service().post(std::bind(&DataProcess::handle_response_message, this, package));
}

void DataProcess::handle_request_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_SymbolData:
            request_symbol_data(package);
            return;    

        case UT_FID_ReqKLineData:
            request_kline_package(package);
            return;
        default:
            break;
    }
    deliver_request(package);
}

void DataProcess::request_symbol_data(PackagePtr package)
{
    std::set<string> symbols;
    for (auto iter:depth_data_)
    {
        symbols.emplace(iter.first);
    }

    PackagePtr package_new = GetNewSymbolDataPackage(symbols, 0);
    package_new->prepare_response(UT_FID_SymbolData, package_new->PackageID());
    deliver_response(package_new);
}

void DataProcess::handle_response_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_SDepthData:
            response_sdepth_package(package);
            return;
        default:
            cout << "Unknow Package" << endl;
            break;
    }    

    deliver_response(package);
}

void DataProcess::response_sdepth_package(PackagePtr package)
{
    try
    {
        SDepthData* p_depth_data = GET_NON_CONST_FIELD(package, SDepthData);

        if (p_depth_data)
        {
            PackagePtr package_new = GetNewEnhancedDepthDataPackage(*p_depth_data, package->PackageID());

            EnhancedDepthData* en_depth_data = GET_NON_CONST_FIELD(package_new, EnhancedDepthData);

            if (depth_data_.find(en_depth_data->depth_data_.symbol) == depth_data_.end())
            {                
                response_new_symbol(en_depth_data->depth_data_.symbol);
            }

            depth_data_[en_depth_data->depth_data_.symbol] = en_depth_data;

            deliver_response(package_new);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DataProcess::response_sdepth_package: " << e.what() << '\n';
    }
}

void DataProcess::response_new_symbol(string symbol)
{
    std::set<string> symbols{symbol};
    PackagePtr package_new = GetNewSymbolDataPackage(symbols, ID_MANAGER->get_id());
    package_new->prepare_response(UT_FID_SymbolData, package_new->PackageID());
    deliver_response(package_new);
}

void DataProcess::request_kline_package(PackagePtr package)
{
    try
    {
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);
        if (pReqKlineData)
        {
            PackagePtr rsp_package = get_kline_package(package);

            if (rsp_package)
            {
                deliver_response(rsp_package);
            }
            else
            {
                deliver_request(package);
            }            
        }
        else
        {
            LOG_ERROR("DataProcess::request_kline_package ReqKLineData NULL!");
        }    
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] DataProcess::request_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void DataProcess::response_src_kline_package(PackagePtr package)
{
    try
    {
        KlineData* p_depth_data = GET_NON_CONST_FIELD(package, KlineData);

        if (p_depth_data)
        {
            kline_data_[p_depth_data->symbol][p_depth_data->index] = p_depth_data;
        }
        else
        {
            LOG_ERROR("DataProcess::response_src_kline_package KlineData ptr is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] DataProcess::response_src_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

PackagePtr DataProcess::get_kline_package(PackagePtr package)
{
    try
    {    
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);

        if (kline_data_.find(pReqKlineData->symbol_) == kline_data_.end())
        {
            std::map<type_tick, KlineData*>& symbol_kline_data = kline_data_[pReqKlineData->symbol_];

            if (symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->end_time_)
            {
                return nullptr;
            }
            else if (symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->start_time_)
            {
                pReqKlineData->append_end_time_ = symbol_kline_data.begin()->first;
                return nullptr;
            }
            else
            {
                std::vector< KlineData*> src_kline_data;
                std::map<type_tick, KlineData*>::iterator  iter = symbol_kline_data.find(pReqKlineData->start_time_);
                while (iter->first <= pReqKlineData->end_time_)
                {
                    src_kline_data.emplace_back(iter->second);
                    ++iter;
                }

                std::vector<AtomKlineDataPtr> target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);

                PackagePtr rsp_package = GetNewRspKLineDataPackage(pReqKlineData, target_kline_data, ID_MANAGER->get_id());

                return rsp_package;                
            }        
        }
        else
        {
            return nullptr;
        }    
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] DataProcess::get_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}
