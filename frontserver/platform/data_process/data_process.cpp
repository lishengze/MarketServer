#include "data_process.h"
#include "hub_interface.h"
#include "pandora/util/time_util.h"
#include "../data_structure/data_struct.h"
#include "../util/tools.h"
#include "../front_server_declare.h"
#include "../log/log.h"
#include "../config/config.h"

DataProcess::DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{
    frequency_list_ = CONFIG->get_frequency_list();
    frequency_numb_ = CONFIG->get_frequency_numb();
    frequency_base_ = CONFIG->get_frequency_base();

    cout << "frequency_numb_: " << frequency_numb_ << endl;

    if (test_kline_data_)
    {
        init_test_kline_data();
    }
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
    handle_response_message(package);

    // get_io_service().post(std::bind(&DataProcess::handle_response_message, this, package));
}

void DataProcess::handle_request_message(PackagePtr package)
{
    cout << "DataProcess::handle_request_message: " << package->Tid() << endl;

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
    // deliver_request(package);
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
            response_src_sdepth_package(package);
            return;

        case UT_FID_KlineData:
            response_src_kline_package(package);
            return;

        default:
            cout << "Unknow Package" << endl;
            break;
    }    

    deliver_response(package);
}

void DataProcess::response_src_sdepth_package(PackagePtr package)
{
    try
    {
        // cout << "DataProcess::response_src_sdepth_package 0" << endl;

        SDepthData* p_depth_data = GET_NON_CONST_FIELD(package, SDepthData);

        if (p_depth_data)
        {
            // cout << "DataProcess::response_src_sdepth_package 1" << endl;

            PackagePtr enhanced_data_package = GetNewEnhancedDepthDataPackage(*p_depth_data, package->PackageID());

            // cout << "DataProcess::response_src_sdepth_package 2" << endl;

            EnhancedDepthData* en_depth_data = GET_NON_CONST_FIELD(enhanced_data_package, EnhancedDepthData);

            // EnhancedDepthDataPtr cur_enhanced_data = boost::make_shared<EnhancedDepthData>(*en_depth_data);

            depth_data_[en_depth_data->depth_data_.symbol] = enhanced_data_package;

            if (depth_data_.find(en_depth_data->depth_data_.symbol) == depth_data_.end())
            {                
                response_new_symbol(en_depth_data->depth_data_.symbol);
            }

            // cout << "DataProcess::response_src_sdepth_package 4" << endl;

            deliver_response(enhanced_data_package);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"DataProcess::response_src_sdepth_package: " << e.what() << '\n';
    }
}

void DataProcess::response_new_symbol(string symbol)
{
    // cout << "DataProcess::response_new_symbol 0" << endl;
    std::set<string> symbols{symbol};

    // cout << "DataProcess::response_new_symbol 1" << endl;
    PackagePtr package_new = GetNewSymbolDataPackage(symbols, ID_MANAGER->get_id());

    // cout << "DataProcess::response_new_symbol 2" << endl;
    package_new->prepare_response(UT_FID_SymbolData, package_new->PackageID());
    deliver_response(package_new);
}

void DataProcess::request_kline_package(PackagePtr package)
{
    try
    {
        cout << "DataProcess::request_kline_package " << endl;
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);
        if (pReqKlineData)
        {
            PackagePtr rsp_package = get_kline_package(package);

            if (rsp_package)
            {
                cout << "deliver_response " << endl;
                deliver_response(rsp_package);
            }
            else
            {
                cout << "Error!" << endl;
                // deliver_request(package);
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
        if (test_kline_data_) return;

        KlineData* pkline_data = GET_NON_CONST_FIELD(package, KlineData);


        if (pkline_data)
        {

            for (int cur_frequency: frequency_list_)
            {
                store_kline_data(cur_frequency, pkline_data);
            }

            // LOG_DEBUG(string("kline_data push ") + pkline_data->symbol + ", "  
            //             + std::to_string(frequency_list_[0]) + ":" + std::to_string(kline_data_[pkline_data->symbol][frequency_list_[0]].size()) + ", "
            //             + utrade::pandora::ToSecondStr(pkline_data->index * NanoPerSec, "%Y-%m-%d %H:%M:%S"));
            // cout << endl;

            if (kline_data_[pkline_data->symbol][frequency_base_].size() % 60 == 0)
            {

                std::map<int, std::map<type_tick, KlineDataPtr>>& cur_kline_data = kline_data_[pkline_data->symbol];

                for (auto iter: cur_kline_data)
                {
                    std::map<type_tick, KlineDataPtr>& cur_fre_data = iter.second;

                    cout << "Frequency: " << iter.first << ", data_numb: " << cur_fre_data.size() << endl;

                    for (auto atom_iter:cur_fre_data)
                    {
                        KlineData& kline = *(atom_iter.second);
                        string cur_time = get_sec_time_str(kline.index);

                        cout << cur_time << ", " << kline.symbol << ", "
                            << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                            << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << endl;
                        
                    }
                }
            }
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

void DataProcess::store_kline_data(int frequency, KlineData* pkline_data)
{
    try
    {
        if (kline_data_.find(pkline_data->symbol) == kline_data_.end() 
        || kline_data_[pkline_data->symbol].find(frequency) == kline_data_[pkline_data->symbol].end())
        {
            KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*pkline_data);
            kline_data_[pkline_data->symbol][frequency][pkline_data->index] = cur_kline_data;

            KlineDataPtr last_kline_data = boost::make_shared<KlineData>(*pkline_data);
            cur_kline_data_[pkline_data->symbol][frequency] = last_kline_data;

            // cout << "set new last_kline: "<< get_sec_time_str(pkline_data->index) << " "
            //      << ", cur_time: " << " "  << get_sec_time_str(pkline_data->index) << endl;

            // cout << "last info: time: " << get_sec_time_str(last_kline_data->index)
            //         <<  " open: " << last_kline_data->px_open.get_value() 
            //         << " high: " << last_kline_data->px_high.get_value() 
            //         << " low: " << last_kline_data->px_low.get_value()  
            //         << " close: " << last_kline_data->px_close.get_value() << "\n" << endl;                  

            KlineDataPtr last_kline = cur_kline_data_[pkline_data->symbol][frequency];
              
        }
        else
        {
            type_tick cur_time = pkline_data->index;
            type_tick last_update_time = kline_data_[pkline_data->symbol][frequency].rbegin()->second->index;

            // cout << "cur_time: " << get_sec_time_str(cur_time) << ", last_time: " << get_sec_time_str(last_update_time) << endl;

            KlineDataPtr last_kline = cur_kline_data_[pkline_data->symbol][frequency];

            if (last_kline->is_clear())
            {                
                last_kline->reset(*pkline_data);

                // cout << "Set New: "<< get_sec_time_str(last_kline->index) 
                //      << " open: " << last_kline->px_open.get_value() 
                //      << " high: " << last_kline->px_high.get_value() 
                //      << " low: " << last_kline->px_low.get_value()  
                //      << " close: " << last_kline->px_close.get_value() << endl; 
            }
            else
            {
                // cout << "Com old: " << get_sec_time_str(last_kline->index)
                //      << " open: " << last_kline->px_open.get_value() 
                //      << " high: " << last_kline->px_high.get_value() 
                //      << " low: " << last_kline->px_low.get_value()  
                //      << " close: " << last_kline->px_close.get_value() << endl; 

                // cout << "cur inf: " << get_sec_time_str(pkline_data->index)
                //      << " open: " << pkline_data->px_open.get_value() 
                //      << " high: " << pkline_data->px_high.get_value() 
                //      << " low: " << pkline_data->px_low.get_value()  
                //      << " close: " << pkline_data->px_close.get_value() << endl;  

                last_kline->px_close = pkline_data->px_close;
                last_kline->px_low = last_kline->px_low > pkline_data->px_low ? pkline_data->px_low: last_kline->px_low;
                last_kline->px_high = last_kline->px_high < pkline_data->px_high ? pkline_data->px_high: last_kline->px_high;
                last_kline->index = pkline_data->index;
                last_kline->symbol = pkline_data->symbol;                
            }

            if ((cur_time - last_update_time) % frequency == 0)
            {
                // cout << "Add data: " << "cur_time: " << get_sec_time_str(cur_time) 
                //      << ", last_update_time: " << get_sec_time_str(last_update_time) << endl;

                if (kline_data_[pkline_data->symbol][frequency].size() == frequency_numb_)
                {
                    kline_data_[pkline_data->symbol][frequency].erase(kline_data_[pkline_data->symbol][frequency].begin());
                }

                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                kline_data_[pkline_data->symbol][frequency][cur_time] = cur_kline_data;

                // cout << "Add inf: " << get_sec_time_str(last_kline->index)
                //         << " open: " << last_kline->px_open.get_value() 
                //         << " high: " << last_kline->px_high.get_value() 
                //         << " low: " << last_kline->px_low.get_value()  
                //         << " close: " << last_kline->px_close.get_value() << "\n"
                //         << endl;      

                last_kline->clear();                                   
            }
        }

        // cout << "Lastest Time: " << get_sec_time_str(kline_data_[pkline_data->symbol][frequency].begin()->first) << " " 
        //      <<get_sec_time_str(kline_data_[pkline_data->symbol][frequency].begin()->second->index) << endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

PackagePtr DataProcess::get_kline_package(PackagePtr package)
{
    cout << "DataProcess::get_kline_package  " << endl;
    try
    {    
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);

        cout << "symbol_: " << pReqKlineData->symbol_ << ", \n"
             << "frequency_: " << pReqKlineData->frequency_ << ", \n"
             << "request start_time: " << pReqKlineData->start_time_ << ", \n"
             << "request end_time: " << pReqKlineData->end_time_ << ", \n"
             << endl;

        if (kline_data_.find(pReqKlineData->symbol_) != kline_data_.end())
        {
            std::map<type_tick, KlineDataPtr>& symbol_kline_data = kline_data_[pReqKlineData->symbol_][frequency_base_];

            if (kline_data_[pReqKlineData->symbol_].find(pReqKlineData->frequency_) != kline_data_[pReqKlineData->symbol_].end())
            {
                cout << "Has frequency_: " << pReqKlineData->frequency_ << endl;
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_];
            }

            cout << "\n" << pReqKlineData->symbol_ << ", \n"
                 << "kline_data start_time: " << symbol_kline_data.begin()->first << ", \n"
                 << "kline_data end_time: " << symbol_kline_data.rbegin()->first << ", \n"
                 << endl;

            if (symbol_kline_data.begin()->first > pReqKlineData->end_time_)
            {
                LOG_ERROR("symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->end_time_");
                return nullptr;
            }
            else if (symbol_kline_data.begin()->first > pReqKlineData->start_time_)
            {
                LOG_ERROR("symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->start_time_");
                pReqKlineData->append_end_time_ = symbol_kline_data.begin()->first;
                return nullptr;
            }
            else
            {
                cout << " Compute!  " << endl;

                std::vector< KlineDataPtr> src_kline_data;

                std::map<type_tick, KlineDataPtr>::iterator  iter = symbol_kline_data.begin();

                while(iter->first <= pReqKlineData->start_time_ && (++iter != symbol_kline_data.end() && iter->first > pReqKlineData->start_time_))
                {
                    break;
                }

                if (iter == symbol_kline_data.end()) 
                {
                    LOG_ERROR("Kline Data Error");
                    return nullptr;                    
                }                

                cout << "Real Start Time: " << iter->first << endl;

                while (iter->first <= pReqKlineData->end_time_ && iter != symbol_kline_data.end())
                {
                    src_kline_data.emplace_back(iter->second);                    
                    // cout << iter->first << endl;                    
                    ++iter;
                }

                cout << "src_kline_data data numb: " << src_kline_data.size() << endl;     

                std::vector<AtomKlineDataPtr> target_kline_data;           
                
                // target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);
                
                cout << "DataProcess::compute_target_kline_data" << endl;

                std::vector<AtomKlineDataPtr> result;

                if (src_kline_data.size() == 0) return nullptr;

                AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*(src_kline_data[0]));
                cout << "\ncur_data.tick: " << cur_data->tick_ << endl;

                result.push_back(cur_data); 

                double low = MAX_DOUBLE;
                double high = MIN_DOUBLE;

                src_kline_data.erase(src_kline_data.begin());

                cout << "kline_data.size: " << src_kline_data.size() << endl;

                for (KlineDataPtr atom : src_kline_data)
                {
                    // cout << "atom.tick " << atom->index << endl;
                    low = low > atom->px_low.get_value() ? atom->px_low.get_value() : low;
                    high = high < atom->px_high.get_value() ? atom->px_high.get_value():high;

                    if (atom->index - (*result.rbegin())->tick_ >= pReqKlineData->frequency_)
                    {            
                        AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*atom);
                        cur_data->low_ = low;
                        cur_data->high_ = high;
                        result.push_back(cur_data); 

                        low = MAX_DOUBLE;
                        high = MIN_DOUBLE;
                    }
                }

                target_kline_data = result;

                for (AtomKlineDataPtr atom_data:target_kline_data)
                {
                    cout << atom_data->tick_ << endl;
                }

                cout << "compute_target_kline_data done " << endl;

                PackagePtr rsp_package = GetNewRspKLineDataPackage(pReqKlineData, target_kline_data, ID_MANAGER->get_id());

                cout << "GetNewRspKLineDataPackage done " << endl;

                rsp_package->prepare_response(UT_FID_RspKLineData, rsp_package->PackageID());

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

/*
// std::vector<AtomKlineDataPtr>& DataProcess::compute_target_kline_data(std::vector< KlineData*>& kline_data, int frequency)
// {
//     cout << "DataProcess::compute_target_kline_data" << endl;

//     std::vector<AtomKlineDataPtr> result;

//     cout << "kline_data.size: " << kline_data.size() << endl;

//     if (kline_data.size() == 0) return result;

//     AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*(kline_data[0]));
//     cout << "\ncur_data.tick: " << cur_data->tick_ << endl;
//     // AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>();

//     cout << "make over" << endl;

//     result.push_back(cur_data); 

//     double low = MAX_DOUBLE;
//     double high = MIN_DOUBLE;

//     kline_data.erase(kline_data.begin());

//     cout << "kline_data.size: " << kline_data.size() << endl;

//     for (KlineData* atom : kline_data)
//     {
//         cout << "atom.tick " << atom->index << endl;
//         low = low > atom->px_low.get_value() ? atom->px_low.get_value() : low;
//         high = high < atom->px_high.get_value() ? atom->px_high.get_value():high;

//         if (atom->index - (*result.rbegin())->tick_ >= frequency)
//         {            
//             AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*atom);
//             cur_data->low_ = low;
//             cur_data->high_ = high;
//             result.push_back(cur_data); 

//             low = MAX_DOUBLE;
//             high = MIN_DOUBLE;
//         }
//     }

//     cout << "compute over" << endl;

//     return result;
// }
*/

// Creaet Last Hour Test Data For BTC_USDT
void DataProcess::init_test_kline_data()
{
    string symbol = "BTC_USDT";
    int frequency_secs = 60;
    type_tick end_time_secs = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
    end_time_secs = mod_secs(end_time_secs, frequency_secs);

    int test_time_len = 60* 60 *2;

    double test_max = 100;
    double test_min = 10;
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(test_min, test_max);    
    std::uniform_real_distribution<> offset(1,10);

    for (int i = 0; i < test_time_len; ++i)
    {
        type_tick cur_time = end_time_secs - i * frequency_secs;

        double open = dis(gen);
        double close = dis(gen);
        double high = std::max(open, close) + offset(gen);
        double low = std::min(open, close) - offset(gen);
        double volume = dis(gen) * 5;

        // boost::shared_ptr<KlineData> cur_kline_data = boost::make_shared<KlineData>(symbol, cur_time, open, high, low, close,volume);

        // kline_data_[symbol][cur_time] = new KlineData{symbol, cur_time, open, high, low, close, volume};

        kline_data_[symbol][frequency_base_][cur_time] = boost::make_shared<KlineData>(symbol, cur_time, open, high, low, close, volume);
    }

    cout << "test symbol: " << symbol << ", \n"
         << "test frequency_base_: " << frequency_base_ << ", \n"
         << "test start_time: " << kline_data_[symbol][frequency_base_].begin()->first << ", \n"
         << "test end_time: " << kline_data_[symbol][frequency_base_].rbegin()->first << endl;

    cout << "init end!" << endl;
}