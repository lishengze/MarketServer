#include "../front_server_declare.h"
#include "../config/config.h"
#include "../log/log.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../util/id.hpp"

#include "kline_process.h"
#include "hub_interface.h"
#include "data_process.h"

KlineProcess::KlineProcess()
{
    init_config();
}

void KlineProcess::init_config()
{
    frequency_cache_set_ = CONFIG->get_frequency_list();
    frequency_cache_numb_ = CONFIG->get_frequency_numb();
    frequency_base_set_ = CONFIG->get_frequency_base();

    for (int cache_frequency:frequency_cache_set_)
    {
        for (set<int>::reverse_iterator iter = frequency_base_set_.rbegin(); iter != frequency_base_set_.rend(); ++iter)
        {
            if (cache_frequency % *iter == 0)
            {
                frequency_aggreration_map_[cache_frequency] = *iter;
                break;
            }
        }
    }

    cout << "frequency_aggreration_map_ " << endl;
    for (auto iter:frequency_aggreration_map_)
    {
        cout << iter.first << " " << iter.second << endl;
    }
    cout << endl;
}

void KlineProcess::init_process_engine(DataProcessPtr process_engine)
{
    process_engine_ = process_engine;
}

void KlineProcess::request_kline_package(PackagePtr package)
{
    try
    {
        cout << "KlineProcess::request_kline_package " << endl;

        ReqKLineDataPtr pReqKlineData = GetField<ReqKLineData>(package);

        if (pReqKlineData)
        {
            if(pReqKlineData -> is_canacel_request_)
            {
                delete_kline_request_connect(pReqKlineData->symbol_, pReqKlineData->socket_id_);
            }
            else
            {
                check_websocket_subinfo(pReqKlineData);

                PackagePtr rsp_package = get_kline_package(package);

                if (rsp_package)
                {
                    init_update_kline_data(rsp_package, pReqKlineData);

                    cout << "deliver_response " << endl;
                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    cout << "Error!" << endl;
                    // process_engine_->deliver_request(package);
                }     
            }       
        }
        else
        {
            LOG_ERROR("KlineProcess::request_kline_package ReqKLineData NULL!");
        }    
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::request_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::request_kline_package Unknow Exceptions! " << "\n";
        LOG_ERROR(stream_obj.str());        
    }
}

void KlineProcess::request_trade_package(PackagePtr package)
{
    try
    {
        ReqTradePtr pReqTradePtr = GetField<ReqTrade>(package);

        if (pReqTradePtr)
        {
            check_websocket_trade_req(pReqTradePtr);

            PackagePtr package = get_trade_package(pReqTradePtr);

            if (package)
            {
                process_engine_->deliver_response(package);
            }
            else
            {
                LOG_ERROR("KlineProcess::request_trade_package get_trade_package Failed!");
            }
        }
        else
        {
            LOG_ERROR("KlineProcess::request_trade_package GetField<ReqTrade> Failed!");
        }
     
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::request_trade_package " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::request_trade_package: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void KlineProcess::response_src_kline_package(PackagePtr package)
{
    try
    {
        if (test_kline_data_) return;

        KlineDataPtr pkline_data = GetField<KlineData>(package);

        update_kline_data(pkline_data);

        if (pkline_data)
        {
            string cur_symbol = string(pkline_data->symbol);
            int src_freq = pkline_data->frequency_;

            for (int cur_frequency: frequency_cache_set_)
            {
                if (src_freq == frequency_aggreration_map_[cur_frequency])
                {
                    bool store_new = store_kline_data(cur_frequency, pkline_data, src_freq);

                    if (cur_frequency == 7200 && false)
                    {
                        std::map<type_tick, KlineDataPtr>& cur_fre_data = kline_data_[cur_symbol][cur_frequency];

                        // LOG_DEBUG(string("kline_data push ") + cur_symbol 
                        //             + ", src_fre: " + std::to_string(src_freq)  
                        //             + ", cur_fre: " + std::to_string(cur_frequency)
                        //             + ", size: " + std::to_string(kline_data_[cur_symbol][cur_frequency].size()) + ", "
                        //             + get_sec_time_str(pkline_data->index) + "\n");   

                        cout << get_sec_time_str(pkline_data->index) << ", " << pkline_data->symbol << ", "
                                << "open: " << pkline_data->px_open.get_value() << ", high: " << pkline_data->px_high.get_value() << ", "
                                << "low: " << pkline_data->px_low.get_value() << ", close: " << pkline_data->px_close.get_value()
                                << endl;   

                        cout << "detailed data" << endl;
                        for (auto atom_iter:cur_fre_data)
                        {
                            
                            KlineData& kline = *(atom_iter.second);
                            string cur_time = get_sec_time_str(kline.index);

                            cout << cur_time << ", " << kline.symbol << ", "
                                 << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                                 << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << endl;                        
                        }
                        cout << endl;
                     
                    }
                }                
            }

            // if (kline_data_[cur_symbol][frequency_cache_set_[0]].size() > 60) return;            

            // LOG_DEBUG(string("kline_data push ") + cur_symbol + ", "  
            //             + std::to_string(frequency_cache_set_[0]) + ":" + std::to_string(kline_data_[cur_symbol][frequency_cache_set_[0]].size()) + ", "
            //             + get_sec_time_str(pkline_data->index) + "\n");

            // for (auto fre:frequency_cache_set_)
            // {
            //     LOG_DEBUG(string("Push ") + cur_symbol + ", "  
            //                 + std::to_string(fre) + ":" + std::to_string(kline_data_[cur_symbol][fre].size()) + ", "
            //                 + get_sec_time_str(pkline_data->index));                
            // }

            // if (kline_data_[cur_symbol][300].size() == 100)
            // {

            //     std::map<int, std::map<type_tick, KlineDataPtr>>& cur_kline_data = kline_data_[cur_symbol];

            //     for (auto iter: cur_kline_data)
            //     {
            //         std::map<type_tick, KlineDataPtr>& cur_fre_data = iter.second;

            //         cout << "Frequency: " << iter.first << ", data_numb: " << cur_fre_data.size() << endl;

            //         for (auto atom_iter:cur_fre_data)
            //         {
            //             KlineData& kline = *(atom_iter.second);
            //             string cur_time = get_sec_time_str(kline.index);

            //             cout << cur_time << ", " << kline.symbol << ", "
            //                 << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
            //                 << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << endl;                        
            //         }
            //     }
            // }
        }
        else
        {
            LOG_ERROR("KlineProcess::response_src_kline_package KlineData ptr is NULL!");
        }
        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::response_src_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void KlineProcess::response_src_trade_package(PackagePtr package)
{
    try
    {
        TradeDataPtr pTradeDataPtr = GetField<TradeData>(package);
        if (pTradeDataPtr)
        {
            std::lock_guard<std::mutex> lk(trade_data_map_mutex_);
            trade_data_map_[string(pTradeDataPtr->symbol_)] = pTradeDataPtr;
        }
        else 
        {
            LOG_ERROR("KlineProcess::response_src_trade_package GetField Failed!");
        }        
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::response_src_trade_package " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::response_src_trade_package: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

bool KlineProcess::store_kline_data(int frequency, KlineDataPtr pkline_data, int base_frequency)
{
    try
    {
        std::lock_guard<std::mutex> lk(kline_data_mutex_);

        string cur_symbol = string(pkline_data->symbol);

        if (kline_data_.find(cur_symbol) == kline_data_.end() 
        || kline_data_[cur_symbol].find(frequency) == kline_data_[cur_symbol].end())
        {
            KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*pkline_data);
            kline_data_[cur_symbol][frequency][pkline_data->index] = cur_kline_data;

            KlineDataPtr last_kline_data = boost::make_shared<KlineData>(*pkline_data);
            cur_kline_data_[cur_symbol][frequency] = last_kline_data;

            // cout << "set new last_kline: "<< get_sec_time_str(pkline_data->index) << " "
            //      << ", cur_time: " << " "  << get_sec_time_str(pkline_data->index) << endl;

            // cout << "last info: time: " << get_sec_time_str(last_kline_data->index)
            //         <<  " open: " << last_kline_data->px_open.get_value() 
            //         << " high: " << last_kline_data->px_high.get_value() 
            //         << " low: " << last_kline_data->px_low.get_value()  
            //         << " close: " << last_kline_data->px_close.get_value() << "\n" << endl;                  

            KlineDataPtr last_kline = cur_kline_data_[cur_symbol][frequency];
              
        }
        else
        {
            type_tick cur_time = pkline_data->index;
            type_tick last_update_time = kline_data_[cur_symbol][frequency].rbegin()->second->index;

            if (cur_time < last_update_time)
            {
                std::stringstream stream_obj;
                stream_obj  << "[Kine] Time Seq is Error , "<< cur_symbol << " current time is " << get_sec_time_str(cur_time)
                            << ", last update time is " << get_sec_time_str(last_update_time) << "\n";

                LOG_ERROR(stream_obj.str());

                return false;
            }
            else if(cur_time == last_update_time)
            {

            }

            // cout << "cur_time: " << get_sec_time_str(cur_time) << ", last_time: " << get_sec_time_str(last_update_time) << endl;

            KlineDataPtr last_kline = cur_kline_data_[cur_symbol][frequency];

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
                assign(last_kline->symbol, cur_symbol);
            }

            if (cur_time - last_update_time >= frequency)
            {
                // cout << "Add data: " << "cur_time: " << get_sec_time_str(cur_time) 
                //      << ", last_update_time: " << get_sec_time_str(last_update_time) << endl;

                if (kline_data_[cur_symbol][frequency].size() == frequency_cache_numb_)
                {
                    kline_data_[cur_symbol][frequency].erase(kline_data_[cur_symbol][frequency].begin());
                }

                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                kline_data_[cur_symbol][frequency][cur_time] = cur_kline_data;

                // cout << "Add inf: " << get_sec_time_str(last_kline->index)
                //         << " open: " << last_kline->px_open.get_value() 
                //         << " high: " << last_kline->px_high.get_value() 
                //         << " low: " << last_kline->px_low.get_value()  
                //         << " close: " << last_kline->px_close.get_value() << "\n"
                //         << endl;      

                last_kline->clear();   

                return true;                                
            }
        }

        return false;

        // cout << "Lastest Time: " << get_sec_time_str(kline_data_[cur_symbol][frequency].begin()->first) << " " 
        //      <<get_sec_time_str(kline_data_[cur_symbol][frequency].begin()->second->index) << endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

PackagePtr KlineProcess::get_kline_package(PackagePtr package)
{
    cout << "KlineProcess::get_kline_package  " << endl;
    try
    {  
        ReqKLineDataPtr pReqKlineData = GetField<ReqKLineData>(package);

        stringstream s_obj;
        s_obj << "\nsymbol_: " << pReqKlineData->symbol_ << ", \n"
             << "frequency_: " << pReqKlineData->frequency_ << ", \n"
             << "start_time: " << pReqKlineData->start_time_ << ", \n"
             << "end_time: " << pReqKlineData->end_time_ << ", \n"
             << "data_count: " << pReqKlineData->data_count_ << ", \n";
        LOG_DEBUG(s_obj.str());

        if (kline_data_.find(pReqKlineData->symbol_) != kline_data_.end())
        {
            std::lock_guard<std::mutex> lk(kline_data_mutex_);

            std::map<type_tick, KlineDataPtr> symbol_kline_data;

            vector<KlineData> append_result;
            vector<KlineDataPtr> src_kline_data;
            bool is_need_aggregation{true};
            int best_freq_base = get_best_freq_base(pReqKlineData->frequency_);
                        
            cout << "best_freq_base: " << best_freq_base << endl;

            if (kline_data_[pReqKlineData->symbol_].find(pReqKlineData->frequency_) != kline_data_[pReqKlineData->symbol_].end()
            && pReqKlineData->data_count_ <= kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_].size())
            {
                cout << "Has frequency_: " << pReqKlineData->frequency_ << endl;
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_];
                is_need_aggregation = false;
            }
            else
            {                
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][best_freq_base];
            }

            if (is_need_aggregation)
            {
                cout << "\n src info: " << pReqKlineData->symbol_ << ", " 
                    << "fre: " << best_freq_base << ", "
                    << "start: " << get_sec_time_str(symbol_kline_data.begin()->first) << ", "
                    << "end: " << get_sec_time_str(symbol_kline_data.rbegin()->first) << ", "
                    << "count: " << symbol_kline_data.size() << "\n"
                    << endl;
            }
            else
            {
                cout << "\n" << pReqKlineData->symbol_ << ", " 
                    << "src fre: " << pReqKlineData->frequency_ << ", "
                    << "start: " << get_sec_time_str(symbol_kline_data.begin()->first) << ", "
                    << "end: " << get_sec_time_str(symbol_kline_data.rbegin()->first) << ", "
                    << "count: " << symbol_kline_data.size() 
                    << endl;
            }
            
            if (pReqKlineData->data_count_ == -1)
            {
                if (symbol_kline_data.begin()->first > pReqKlineData->end_time_)
                {
                    LOG_ERROR("symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->end_time_");
                    return nullptr;
                }
                else if (symbol_kline_data.begin()->first > pReqKlineData->start_time_)
                {
                    LOG_ERROR("symbol_kline_data.begin()->first - pReqKlineData->frequency_ > pReqKlineData->start_time_");
                    
                    HubInterface::get_kline("", pReqKlineData->symbol_, best_freq_base, pReqKlineData->start_time_, symbol_kline_data.begin()->first, append_result);
                }

                append_kline_to_klinePtr(src_kline_data, append_result);
                get_src_kline_data(src_kline_data, symbol_kline_data, 
                                   pReqKlineData->start_time_, pReqKlineData->end_time_, best_freq_base);
            }
            else
            {
                int data_count = pReqKlineData->data_count_;
                if (is_need_aggregation)
                {
                    data_count *= (pReqKlineData->frequency_ / best_freq_base);
                }
                cout << "real data_cout: " << data_count << endl;
                get_src_kline_data(pReqKlineData->symbol_, src_kline_data, symbol_kline_data, data_count, best_freq_base);
            }            

            cout << "sum_src_kline_data.size: " << src_kline_data.size() << endl;

            vector<KlineDataPtr> target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);

            PackagePtr rsp_package;

            if (target_kline_data.size() > 0)
            {
                rsp_package = GetNewRspKLineDataPackage(*pReqKlineData, target_kline_data, ID_MANAGER->get_id());              
            }
            else
            {
                stringstream s_obj;
                s_obj << "No KLine Data For " << pReqKlineData->symbol_ 
                      << " fre: " << pReqKlineData->frequency_ 
                      << " data_count: " << pReqKlineData->data_count_;

                string err_msg = s_obj.str();
                int err_id = 1;
                rsp_package = GetRspErrMsgPackage(err_msg, err_id, pReqKlineData->socket_id_, pReqKlineData->socket_type_);

                if (rsp_package)
                {

                }
                else
                {
                    LOG_ERROR("KlineProcess::get_kline_package GetRspErrMsgPackage Failed!");
                }
            }
            
            return rsp_package;                       
        }
        else
        {
            LOG_ERROR(string("Symbol: ") + string(pReqKlineData->symbol_) + string(" does not exit!"));
            return nullptr;
        }    
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_kline_package: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void KlineProcess::complete_kline_data(vector<KlineData>& ori_symbol_kline_data, vector<KlineData>& append_result, frequency_type frequency)
{

}

void KlineProcess::get_src_kline_data(vector<KlineDataPtr>& src_kline_data, std::map<type_tick, KlineDataPtr>& symbol_kline_data, 
                                        type_tick start_time, type_tick end_time, int cur_freq_base)
{
    std::map<type_tick, KlineDataPtr>::iterator  iter = symbol_kline_data.begin();

    while(iter->first <= start_time && (++iter != symbol_kline_data.end() && iter->first > start_time))
    {
        break;
    }

    if (iter == symbol_kline_data.end()) 
    {
        LOG_ERROR("Kline Data Error");                
    }                
    else
    {
        cout << "Real Start Time: " << iter->first << endl;

        while (iter->first <= end_time && iter != symbol_kline_data.end())
        {
            src_kline_data.emplace_back(iter->second);       
            ++iter;
        }

        cout << "src_kline_data data numb: " << src_kline_data.size() << endl;   
    }
}

void KlineProcess::get_src_kline_data(string symbol, vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, 
                                        int data_count, int cur_freq_base)
{
    std::map<type_tick, KlineDataPtr>::iterator iter = symbol_kline_data.begin();

    if (data_count > symbol_kline_data.size())
    {
        int cur_earliest_time = symbol_kline_data.begin()->second->index;
        int append_data_count = data_count - symbol_kline_data.size();
        int start_time = cur_earliest_time - append_data_count * cur_freq_base;

        vector<KlineData> append_result;
        HubInterface::get_kline("", symbol.c_str(), cur_freq_base, start_time, cur_earliest_time, append_result);

        cout << "sum_data_count: " <<  data_count << " "
             << "cur_data_count: " << symbol_kline_data.size() << " "
             << "need_append_count: " << append_data_count << " "
             << "recv_append_count: " << append_result.size() << " \n"             
             << "cur_freq_base: " << cur_freq_base << " "
             << "req_start_time: " << get_sec_time_str(start_time) << " "
             << "req_end_time: " << get_sec_time_str(cur_earliest_time) << "\n"
             << endl; 

        cout << "\nrecv_data: " <<endl;
        for (KlineData& kline_data:append_result)
        {
            result.emplace_back(boost::make_shared<KlineData>(kline_data));
            cout << get_sec_time_str(kline_data.index) << " "
                 << kline_data.px_open.get_value() << " "
                 << kline_data.px_close.get_value() << " "
                 << kline_data.px_high.get_value() << " "
                 << kline_data.px_low.get_value() << " "
                 << endl;
        }
        cout << endl;
    }
    else
    {
        while (data_count < symbol_kline_data.size())
        {
            iter++;
            data_count++;
        }
    }

    while (iter != symbol_kline_data.end())
    {
        result.emplace_back(iter->second);       
        ++iter;
    }

    // cout << "\n All Src Data" << endl;
    // for (KlineDataPtr& kline_data:result)
    // {
    //     cout << get_sec_time_str(kline_data->index) << " "
    //             << kline_data->px_open.get_value() << " "
    //             << kline_data->px_close.get_value() << " "
    //             << kline_data->px_high.get_value() << " "
    //             << kline_data->px_low.get_value() << " "
    //             << endl;
    // }    
}

vector<KlineDataPtr> KlineProcess::compute_target_kline_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    vector<KlineDataPtr> result;

    cout << "compute_target_kline_data" << endl;

    KlineDataPtr cur_data = src_kline_data[0];
    // cout << "\ncur_data.tick: " << cur_data->tick_ << endl;

    result.push_back(cur_data); 

    SDecimal low(MAX_DOUBLE);
    SDecimal high(MIN_DOUBLE);
    SDecimal open;

    // cout << "kline_data.size: " << src_kline_data.size() << endl;

    src_kline_data.erase(src_kline_data.begin());

    bool is_first = true;
    for (KlineDataPtr atom:src_kline_data)
    {
        low = low > atom->px_low ? atom->px_low:low;
        high = high < atom->px_high ? atom->px_high:high;

        if (is_first)
        {
            open = atom->px_open;
            is_first = false;
        }
        
        if (atom->index > (*result.rbegin())->index && atom->index - (*result.rbegin())->index >= frequency)
        {            
            KlineDataPtr cur_data = boost::make_shared<KlineData>(*atom);
            cur_data->px_low = low;
            cur_data->px_high = high;
            cur_data->px_open = open;

            is_first = true;

            // cout << "cur_data: " << get_sec_time_str(atom->index) << " "
            //      << "last_data: " << get_sec_time_str((*result.rbegin())->index) << " "
            //      << "delta: " << atom->index - (*result.rbegin())->index << " "
            //      << "fre: " << frequency << endl;

            result.push_back(cur_data); 

            low = MAX_DOUBLE;
            high = MIN_DOUBLE;
        }
    }
    return result;
}

void KlineProcess::init_update_kline_data(PackagePtr rsp_package, ReqKLineDataPtr pReqKlineData)
{
    RspKLineDataPtr pRspKlineData = GetField<RspKLineData>(rsp_package);
    if (pRspKlineData)
    {
        auto iter = pRspKlineData->kline_data_vec_.rbegin();
        KlineDataUpdate kline_update(*pReqKlineData);

        kline_update.last_update_time_ = (*iter)->index;

        cout << "\nInit update_kline_data socket_id: " << pReqKlineData->socket_id_ << " " 
             << pReqKlineData->symbol_ << " Last Update Time: " 
             << get_sec_time_str(kline_update.last_update_time_) << endl; 

        updated_kline_data_map_[string(pReqKlineData->symbol_)].emplace_back(kline_update);          
    }
}

bool KlineProcess::delete_kline_request_connect(string symbol, ID_TYPE socket_id)
{
    try
    {
        cout << "KlineProcess::delete_kline_request_connect " << symbol << " " << socket_id << endl;
        std::lock_guard<std::mutex> lk(updated_kline_data_map_mutex_);
        if (updated_kline_data_map_.find(symbol) != updated_kline_data_map_.end())
        {
            vector<KlineDataUpdate>& kline_updater = updated_kline_data_map_[symbol];

            // int pos = 0;
            // for (; pos < kline_updater.size(); ++pos)
            // {
            //     if (kline_updater[pos].reqkline_data.socket_id_ == socket_id) break;
            // }

            // if (pos != kline_updater.size())
            // {
            //     std::stringstream stream_obj;
            //     stream_obj << "[Kline Update]: Erase Websocket " 
            //             << kline_updater[pos].reqkline_data.symbol_ << " " 
            //             << kline_updater[pos].reqkline_data.frequency_ << " "
            //             << kline_updater[pos].reqkline_data.socket_id_ << "\n";
            //     LOG_DEBUG(stream_obj.str());    

            //     while (pos < kline_updater.size()-1)
            //     {
            //         kline_updater[pos] = kline_updater[pos+1];
            //         ++pos;
                    
            //     }
            //     kline_updater.pop_back();
            // }

            vector<KlineDataUpdate>::iterator iter = kline_updater.begin();
            for(;iter != kline_updater.end(); ++iter)
            {
                if ((*iter).reqkline_data.socket_id_ == socket_id) break;
            }

            if (iter != kline_updater.end())
            {
                std::stringstream stream_obj;
                stream_obj << "[Kline Update]: Erase Websocket " 
                        << (*iter).reqkline_data.symbol_ << " " 
                        << (*iter).reqkline_data.frequency_ << " "
                        << (*iter).reqkline_data.socket_id_ << "\n";
                LOG_DEBUG(stream_obj.str());
                updated_kline_data_map_[symbol].erase(iter);
            }
        }
        return true;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::delete_kline_request_connect: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch (...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::delete_kline_request_connect: Unkonw Exceptions " << "\n";
        LOG_ERROR(stream_obj.str());        
    }
}

void KlineProcess::update_kline_data(const KlineDataPtr kline_data)
{
    string symbol = kline_data->symbol;

    if (updated_kline_data_map_.find(symbol) != updated_kline_data_map_.end())
    {
        cout << "KlineProcess::update_kline_data " << symbol << endl;

        // vector<KlineDataUpdatePtr>& vec = updated_kline_data_map_[symbol];

        // cout << "vec.size: " << vec.size() << endl;

        for (auto& kline_update:updated_kline_data_map_[symbol])
        {            
            int cur_fre = kline_update.reqkline_data.frequency_;

            if (frequency_aggreration_map_[cur_fre] != kline_data->frequency_) continue;

            if (!kline_update.kline_data_)
            {
                kline_update.kline_data_ = boost::make_shared<KlineData>(*kline_data);
            }

            KlineDataPtr& last_kline = kline_update.kline_data_;

            if (last_kline->is_clear())
            {
                last_kline->reset(*kline_data);

                cout << "last_kline: " <<"open: " << last_kline->px_open.get_value() << " " 
                <<"close: " << last_kline->px_close.get_value() << " "
                <<"high: " << last_kline->px_high.get_value() << " "
                <<"low: " << last_kline->px_low.get_value() << " "                
                << " ****"<< endl;
            }
            else
            {
                last_kline->px_close = kline_data->px_close;
                last_kline->px_low = last_kline->px_low > kline_data->px_low ? kline_data->px_low: last_kline->px_low;
                last_kline->px_high = last_kline->px_high < kline_data->px_high ? kline_data->px_high: last_kline->px_high;
                last_kline->index = kline_data->index;
                assign(last_kline->symbol, kline_data->symbol);                
            }

            if (kline_data->index - kline_update.last_update_time_ >= kline_update.reqkline_data.frequency_)
            {
                // 推送更新作为下一个 k 线时间数据;

                // cout << "\n**** Kline Update Time: " << get_sec_time_str(kline_data->index) 
                // <<"open: " << kline_data->px_open.get_value() << " " 
                // <<"close: " << kline_data->px_close.get_value() << " "
                // <<"high: " << kline_data->px_high.get_value() << " "
                // <<"low: " << kline_data->px_low.get_value() << " "                
                // << " ****"<< endl;

                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                cout << "Kline Update New : " << get_sec_time_str(cur_kline_data->index) << " "
                <<"open: " << cur_kline_data->px_open.get_value() << " " 
                <<"close: " << cur_kline_data->px_close.get_value() << " "
                <<"high: " << cur_kline_data->px_high.get_value() << " "
                <<"low: " << cur_kline_data->px_low.get_value() << " "                
                << endl;     

                cout << endl;           
                
                PackagePtr rsp_package = GetNewRspKLineDataPackage(kline_update.reqkline_data, cur_kline_data, ID_MANAGER->get_id());

                if (rsp_package)
                {
                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR("KlineProcess::update_kline_data GetNewRspKLineDataPackage Failed!");
                }

                kline_update.last_update_time_ = kline_data->index;

                last_kline->clear();
            }
            else
            {
                // 推送更新作为当前 k 线时间数据;
                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);
                cur_kline_data->index = kline_update.last_update_time_;

                cout << "Kline Update old : " << get_sec_time_str(cur_kline_data->index) << " "
                <<"open: " << cur_kline_data->px_open.get_value() << " " 
                <<"close: " << cur_kline_data->px_close.get_value() << " "
                <<"high: " << cur_kline_data->px_high.get_value() << " "
                <<"low: " << cur_kline_data->px_low.get_value() << " "                
                << endl;  

                PackagePtr rsp_package = GetNewRspKLineDataPackage(kline_update.reqkline_data, cur_kline_data, ID_MANAGER->get_id());
                if (rsp_package)
                {
                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR("KlineProcess::update_kline_data GetNewRspKLineDataPackage Failed!");
                }

            }
        }
    }
}

void KlineProcess::check_websocket_subinfo(ReqKLineDataPtr pReqKlineData)
{
    cout << "KlineProcess::check_websocket_subinfo " << pReqKlineData->symbol_ <<" " << pReqKlineData->socket_id_ << endl;
    std::lock_guard<std::mutex> lk(wss_con_map_mutex_);
    if (wss_con_map_.find(pReqKlineData->socket_id_) != wss_con_map_.end())
    {
        delete_kline_request_connect(wss_con_map_[pReqKlineData->socket_id_], pReqKlineData->socket_id_);
    }
    wss_con_map_[pReqKlineData->socket_id_] = pReqKlineData->symbol_;
}

void KlineProcess::update_frequency_aggreration_map(int src_fre)
{
    for (int cache_frequency:frequency_cache_set_)
    {
        if (frequency_aggreration_map_.find(cache_frequency) != frequency_aggreration_map_.end())
        {
            if (cache_frequency % src_fre == 0 && frequency_aggreration_map_[cache_frequency] < src_fre)
            {
                frequency_aggreration_map_[cache_frequency] = src_fre;
            }
        }
        else
        {
            frequency_aggreration_map_[cache_frequency] = src_fre;
        }
    }
}

int KlineProcess::get_best_freq_base(int req_frequency)
{
    int result;
    for (std::set<int>::reverse_iterator iter = frequency_base_set_.rbegin(); iter != frequency_base_set_.rend(); ++iter)
    {
        if (req_frequency % *iter == 0)
        {
            result = *iter;
            break;
        } 
    }
    return result;
}

void KlineProcess::init_update_trade_map(ReqTradePtr pReqTrade)
{

}

void KlineProcess::check_websocket_trade_req(ReqTradePtr pReqTrade)
{
    try
    {
        std::lock_guard<std::mutex> lk(trade_wss_con_map_mutex_);
        if (trade_wss_con_map_.find(pReqTrade->socket_id_) != trade_wss_con_map_.end() 
        && string(pReqTrade->symbol_) != trade_wss_con_map_[pReqTrade->socket_id_])
        {
            string ori_symbol = trade_wss_con_map_[pReqTrade->socket_id_];

            std::lock_guard<std::mutex> lk(updated_trade_data_map_mutex_);
            if (updated_trade_data_map_.find(ori_symbol) != updated_trade_data_map_.end()
            && updated_trade_data_map_[ori_symbol].find(pReqTrade->socket_id_) != updated_trade_data_map_[ori_symbol].end())
            {
                updated_trade_data_map_[ori_symbol].erase(pReqTrade->socket_id_);
            }

            string new_symbol = pReqTrade->symbol_;
            TradeDataUpdatePtr pTradeDataUpdate = boost::make_shared<TradeDataUpdate>(*pReqTrade);
            updated_trade_data_map_[new_symbol][pReqTrade->socket_id_] = pTradeDataUpdate;
        }
        else
        {
            trade_wss_con_map_[pReqTrade->socket_id_] = pReqTrade->symbol_;
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::check_websocket_trade_req " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::check_websocket_trade_req: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }       
}

void KlineProcess::update_trade_data(const TradeDataPtr pTradeDataPtr)
{
    try
    {
        string symbol = pTradeDataPtr->symbol_;
        {
            std::lock_guard<std::mutex> lk(trade_data_map_mutex_);
            trade_data_map_[symbol] = pTradeDataPtr;
        }
        
        std::lock_guard<std::mutex> lk(updated_trade_data_map_mutex_);
        if (updated_trade_data_map_.find(symbol) != updated_trade_data_map_.end())
        {
            map<ID_TYPE, TradeDataUpdatePtr>& reqMap = updated_trade_data_map_[symbol];
            for (auto iter:reqMap)
            {
                PackagePtr package = get_trade_package(iter.second->pReqTrade_);
                if (package)
                {
                    process_engine_->deliver_response(package);
                }
                else
                {
                    LOG_ERROR("KlineProcess::request_trade_package get_trade_package Failed!");
                }                
            }
        }    
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::update_trade_data " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::update_trade_data: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

PackagePtr KlineProcess::get_trade_package(ReqTradePtr pReqTrade)
{
    try
    {
        PackagePtr result = nullptr;
        string symbol = pReqTrade->symbol_;
        if (trade_data_map_.find(symbol)!= trade_data_map_.end())
        {
            TradeDataPtr pTradeData = trade_data_map_[symbol];

            int start_time = pTradeData->time_ - 24 * 60 * 60;
            int fre_base = 60;
            std::map<type_tick, KlineDataPtr>& kline_data_map = get_trade_kline_data(fre_base, start_time);

            SDecimal price;
            SDecimal volume;
            SDecimal max_change;
            SDecimal max_change_rate;
            SDecimal high;
            SDecimal low;
        }

        return result;     
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::get_trade_package " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_trade_package: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

std::map<type_tick, KlineDataPtr>& KlineProcess::get_trade_kline_data(int freq,int start_time)
{
    try
    {
        std::map<type_tick, KlineDataPtr> result;

        return result;    
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::get_trade_kline_data " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_trade_kline_data: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }      
}