#include "../front_server_declare.h"
#include "../config/config.h"
#include "../log/log.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../util/id.hpp"

#include "kline_process.h"
#include "hub_interface.h"
#include "data_process.h"

void TimeKlineData::init(type_tick end_time)
{
    try
    {
        int end_time;
        int start_time = end_time - 24 * 60 * 60;
        start_time = mod_secs(start_time, frequency_);

        std::vector<KlineDataPtr> src_kline_data = kline_process_->get_trade_kline_data(symbol_, frequency_, start_time, end_time);

        if (src_kline_data.size() > 0)
        {
            start_price_ = src_kline_data[0]->px_open;

            high_ = src_kline_data[0]->px_high;
            high_time_ = src_kline_data[0]->index;
            low_ = src_kline_data[0]->px_low;
            low_time_ = src_kline_data[0]->index;

            for (auto kline_data:src_kline_data)
            {
                if (high_ < kline_data->px_high)
                {
                    high_ = kline_data->px_high;
                    high_time_ = kline_data->index;
                }

                if (low_ > kline_data->px_low)
                {
                    low_ = kline_data->px_low;
                    low_time_ = kline_data->index;
                }
            }

            std::stringstream s_s;

            s_s << "TimeKlineData::init " << symbol_ << " "
                    << "start_time: " << get_sec_time_str(src_kline_data[0]->index) << " \n"
                    << "end_time: " << get_sec_time_str(src_kline_data[src_kline_data.size()-1]->index) << "\n"
                    << "high_time: " << get_sec_time_str(high_time_) << " high: " << high_.get_value() << "\n"
                    << "low_time: " << get_sec_time_str(low_time_)   << " low: " << low_.get_value() << "\n"
                    << "kline_data.size: " << src_kline_data.size() << " \n";

            LOG_DEBUG(s_s.str());
        }
        else
        {
            std::stringstream s_s;

            s_s << "TimeKlineData::init Get No Kline Source Data \n";

            LOG_DEBUG(s_s.str());
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] TimeKlineData::init " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}

void TimeKlineData::complete(type_tick end_time)
{
    try
    {
        if (ori_data_.size() == 0)
        {
            init(end_time);
        }
        else
        {
            type_tick first_time = ori_data_.begin()->first;
            type_tick last_time = ori_data_.rbegin()->first;

            if (last_time - first_time < last_secs_)
            {
                type_tick req_start_time = last_time - last_secs_;
                type_tick req_end_time = first_time - frequency_;;;

                std::vector<KlineDataPtr> src_kline_data = kline_process_->get_trade_kline_data(symbol_, frequency_, req_start_time, req_end_time);

                for (auto kline_data:src_kline_data)
                {
                    ori_data_[kline_data->index] = kline_data;
                }

                refresh_high_low();

                std::stringstream s_s;
                s_s << "TimeKlineData::complete " << symbol_ << " "
                        << "start_time: " << get_sec_time_str(src_kline_data[0]->index) << " \n"
                        << "end_time: " << get_sec_time_str(src_kline_data[src_kline_data.size()-1]->index) << "\n"
                        << "high_time: " << get_sec_time_str(high_time_) << " high: " << high_.get_value() << "\n"
                        << "low_time: " << get_sec_time_str(low_time_)   << " low: " << low_.get_value() << "\n"
                        << "kline_data.size: " << src_kline_data.size() << " \n";
                LOG_DEBUG(s_s.str());
            }            
        }        
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] TimeKlineData::complete " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}

void TimeKlineData::refresh_high_low()
{
    try
    {
        SDecimal high = ori_data_.begin()->second->px_high;
        SDecimal low = ori_data_.begin()->second->px_low;

        for (auto iter:ori_data_)
        {
            high = high > iter.second->px_high ? high : iter.second->px_high;
            low = low < iter.second->px_low ? low : iter.second->px_low;

            high_time_ = high_ == iter.second->px_high  ? iter.second->index : high_time_;
            low_time_ = low_ == iter.second->px_low ? iter.second->index : low_time_;            
        }

        high_ = high;
        low_ = low;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] TimeKlineData::refresh_high_low " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}

void TimeKlineData::update(KlineDataPtr kline_data)
{
    try
    {
        if (kline_data->frequency_ == frequency_)
        {
            if (ori_data_.size() == 0)
            {
                ori_data_[kline_data->index] = kline_data;

                high_ = kline_data->px_high;
                high_time_ = kline_data->index;

                low_ = kline_data->px_low;
                low_time_ = kline_data->index;
            }
            else 
            {
                type_tick first_time = ori_data_.begin()->first;
                type_tick last_time = ori_data_.rbegin()->first;
                type_tick curr_time = kline_data->index;

                if (curr_time - last_time >= frequency_)
                {
                    ori_data_[curr_time] = kline_data;

                    if (curr_time - first_time > last_secs_ + frequency_)
                    {
                        ori_data_.erase(first_time);
                    }

                    refresh_high_low();
                }
                else
                {
                    high_ = high_ > kline_data->px_high ? high_ : kline_data->px_high;
                    low_ = low_ < kline_data->px_low ? low_ : kline_data->px_low;

                    high_time_ = high_ == kline_data->px_high ? kline_data->index : high_time_;
                    low_time_ = low_ == kline_data->px_low ? kline_data->index : low_time_;
                }
            }

            start_price_ = ori_data_.begin()->second->px_open;            
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] TimeKlineData::update " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

bool TimeKlineData::is_full() 
{
    bool result = false;

    if (ori_data_.size() > 2)
    {
        result =  (ori_data_.rbegin()->first - ori_data_.begin()->first) >= last_secs_;
    }
    return result;
}

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
}

void KlineProcess::init_process_engine(DataProcessPtr process_engine)
{
    process_engine_ = process_engine;
}

void KlineProcess::request_kline_package(PackagePtr package)
{
    try
    {

        ReqKLineDataPtr pReqKlineData = GetField<ReqKLineData>(package);

        if (pReqKlineData)
        {

            LOG_DEBUG(pReqKlineData->str());

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

                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR("KlineProcess::request_kline_package get_kline_package Failed");
                    
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

void KlineProcess::response_src_kline_package(PackagePtr package)
{
    try
    {
        if (test_kline_data_) return;

        KlineDataPtr pkline_data = GetField<KlineData>(package);

        if (pkline_data)
        {
            string cur_symbol = string(pkline_data->symbol);
            int src_freq = pkline_data->frequency_;

            KlineDataPtr update_for_trade = boost::make_shared<KlineData>(*pkline_data);
            update_one_day_kline_data(update_for_trade);

            KlineDataPtr update_for_kline = boost::make_shared<KlineData>(*pkline_data);
            update_kline_data(update_for_kline);

            for (int cur_frequency: frequency_cache_set_)
            {
                if (src_freq == frequency_aggreration_map_[cur_frequency])
                {
                    bool store_new = store_kline_data(cur_frequency, pkline_data, src_freq);

                }                
            }
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

void KlineProcess::update_one_day_kline_data(const KlineDataPtr kline_data)
{
    try
    {
        string cur_symbol = string(kline_data->symbol);

        if (one_day_kline_data_.find(cur_symbol) == one_day_kline_data_.end())
        {
            one_day_kline_data_[cur_symbol] = TimeKlineData(60, 60*60*24, cur_symbol, this);
        }
        else
        {
            one_day_kline_data_[cur_symbol].update(kline_data);
        }

    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::update_one_day_kline_data: " << e.what() << "\n";
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
        || kline_data_[cur_symbol].find(frequency) == kline_data_[cur_symbol].end()
        || frequency == pkline_data->frequency_)
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

            return true;
              
        }
        else
        {
            type_tick cur_time = pkline_data->index;
            type_tick last_update_time = kline_data_[cur_symbol][frequency].rbegin()->second->index;

            if (cur_time < last_update_time)
            {
                std::stringstream stream_obj;
                stream_obj  << "[K-Kine] Time Seq is Error , "<< cur_symbol << " current time is " << get_sec_time_str(cur_time)
                            << ", last update time is " << get_sec_time_str(last_update_time) << "\n";

                LOG_ERROR(stream_obj.str());

                return false;
            }
            else if(cur_time == last_update_time)
            {
                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*pkline_data);
                kline_data_[cur_symbol][frequency][pkline_data->index] = cur_kline_data;

                return true;
            }
            else
            {
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

                if (cur_time - last_update_time >= frequency || cur_time % frequency == 0)
                {
                    // cout << "Add data: " << "cur_time: " << get_sec_time_str(cur_time) 
                    //      << ", last_update_time: " << get_sec_time_str(last_update_time) << endl;

                    if (kline_data_[cur_symbol][frequency].size() > frequency_cache_numb_ * 2)
                    {
                        // kline_data_[cur_symbol][frequency].erase(kline_data_[cur_symbol][frequency].begin());

                        // kline_data_[cur_symbol][frequency].erase()

                        std::map<type_tick, KlineDataPtr>::iterator erase_end_iter =  kline_data_[cur_symbol][frequency].begin();

                        for (int i = 0; i < frequency_cache_numb_; ++i)
                        {
                            erase_end_iter++; 
                        }
                        
                        kline_data_[cur_symbol][frequency].erase(kline_data_[cur_symbol][frequency].begin(), erase_end_iter);
                    }

                    KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                    kline_data_[cur_symbol][frequency][cur_time] = cur_kline_data;

                    last_kline->clear();
                    return true;                                
                }
            }
        }

        return false;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::store_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

PackagePtr KlineProcess::get_kline_package(PackagePtr package)
{
    try
    {  
        ReqKLineDataPtr pReqKlineData = GetField<ReqKLineData>(package);

        stringstream s_obj;
        // s_obj << "\nreq symbol_: " << pReqKlineData->symbol_ << ", \n"
        //      << "frequency_: " << pReqKlineData->frequency_ << ", \n"
        //      << "start_time: " << pReqKlineData->start_time_ << ", \n"
        //      << "end_time: " << pReqKlineData->end_time_ << ", \n"
        //      << "data_count: " << pReqKlineData->data_count_ << ", \n";
        // LOG_DEBUG(s_obj.str());

        if (kline_data_.find(pReqKlineData->symbol_) != kline_data_.end())
        {
            std::lock_guard<std::mutex> lk(kline_data_mutex_);

            std::map<type_tick, KlineDataPtr> symbol_kline_data;

            vector<KlineData> append_result;
            vector<KlineDataPtr> src_kline_data;
            bool is_need_aggregation{true};
            int best_freq_base = get_best_freq_base(pReqKlineData->frequency_);
                        
            if (kline_data_[pReqKlineData->symbol_].find(pReqKlineData->frequency_) != kline_data_[pReqKlineData->symbol_].end()
            && pReqKlineData->data_count_ <= kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_].size())
            {
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_];
                is_need_aggregation = false;
            }
            else
            {                
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][best_freq_base];
            }

            if (is_need_aggregation)
            {
                s_obj << "SRC Kline Info: " << pReqKlineData->symbol_ << ", " 
                    << "fre: " << best_freq_base << ", "
                    << "start: " << get_sec_time_str(symbol_kline_data.begin()->first) << ", "
                    << "end: " << get_sec_time_str(symbol_kline_data.rbegin()->first) << ", "
                    << "count: " << symbol_kline_data.size() << "\n"
                    << endl;
            }
            else
            {
                s_obj << "SRC Kline Info: " << pReqKlineData->symbol_ << ", " 
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
                s_obj << "Needed Full SrcData Count: " << data_count << endl;
                get_src_kline_data(pReqKlineData->symbol_, src_kline_data, symbol_kline_data, data_count, best_freq_base);
            }            

            s_obj << "Sum Kline Data Size: " << src_kline_data.size() << endl;
            LOG_DEBUG(s_obj.str());

            // for (auto kline_data:src_kline_data)
            // {
            //     cout << kline_data->symbol << " " << get_sec_time_str(kline_data->index) << " "
            //         << kline_data->px_open.get_value() << " "
            //         << kline_data->px_close.get_value() << " "
            //         << kline_data->px_high.get_value() << " "
            //         << kline_data->px_low.get_value() << " "
            //         << endl;    
            // }

            vector<KlineDataPtr> target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);

            PackagePtr rsp_package;

            if (target_kline_data.size() > 0)
            {
                // cout << "target_kline_data Data" << endl;

                // if (strcmp(pReqKlineData->symbol_, "BTC_USDT") == 0)
                // {
                //     MaxMinKlineInfo max_min_kline_info_60;
                //     max_min_kline_info_60.px_high = MIN_DOUBLE;
                //     max_min_kline_info_60.px_low = MAX_DOUBLE;
                //     max_min_kline_info_60.symbol = "BTC_USDT";     

                //     for (auto kline_data:target_kline_data)
                //     {
                //         // cout << kline_data->symbol << " " << get_sec_time_str(kline_data->index) << " "
                //         //     << kline_data->px_open.get_value() << " "
                //         //     << kline_data->px_close.get_value() << " "
                //         //     << kline_data->px_high.get_value() << " "
                //         //     << kline_data->px_low.get_value() << " "
                //         //     << endl;    

                //         if (max_min_kline_info_60.px_high < kline_data->px_high)
                //         {
                //             max_min_kline_info_60.px_high = kline_data->px_high;
                //             max_min_kline_info_60.high_time = kline_data->index;
                //         }

                //         if (max_min_kline_info_60.px_low > kline_data->px_low)
                //         {
                //             max_min_kline_info_60.px_low = kline_data->px_low;
                //             max_min_kline_info_60.low_time = kline_data->index;
                //         }                                            
                //     }


                //     // cout << "KlineRsp: " << pReqKlineData->symbol_ << " high: " << max_min_kline_info_60.px_high.get_value() << " time: " << get_sec_time_str(max_min_kline_info_60.high_time)
                //     //     << " low: " << max_min_kline_info_60.px_low.get_value() << " time: " << get_sec_time_str(max_min_kline_info_60.low_time)
                //     //     << endl;
                // }

                rsp_package = GetNewRspKLineDataPackage(*pReqKlineData, target_kline_data, ID_MANAGER->get_id());              
            }
            else
            {
                stringstream s_obj;
                s_obj << "No KLine Data For " << pReqKlineData->symbol_ 
                      << " fre: " << pReqKlineData->frequency_ 
                      << " data_count: " << pReqKlineData->data_count_;
                LOG_DEBUG(s_obj.str());

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
    try
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
            LOG_DEBUG("Real Start Time: " + std::to_string(iter->first));

            while (iter->first <= end_time && iter != symbol_kline_data.end())
            {
                src_kline_data.emplace_back(iter->second);       
                ++iter;
            }
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_src_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void KlineProcess::get_src_kline_data(string symbol, vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, 
                                        int data_count, int cur_freq_base)
{
    try
    {
        std::map<type_tick, KlineDataPtr>::iterator iter = symbol_kline_data.begin();

        if (data_count > symbol_kline_data.size())
        {
            int cur_earliest_time = symbol_kline_data.begin()->second->index;
            int append_data_count = data_count - symbol_kline_data.size();
            int start_time = cur_earliest_time - append_data_count * cur_freq_base;

            vector<KlineData> append_result;
            HubInterface::get_kline("", symbol.c_str(), cur_freq_base, start_time, cur_earliest_time, append_result);

            std::stringstream s_s;
            s_s << "sum_data_count: " <<  data_count << " "
                << "cur_data_count: " << symbol_kline_data.size() << " "
                << "need_append_count: " << append_data_count << " "
                << "recv_append_count: " << append_result.size() << " \n"             
                << "cur_freq_base: " << cur_freq_base << " "
                << "req_start_time: " << get_sec_time_str(start_time) << " "
                << "req_end_time: " << get_sec_time_str(cur_earliest_time) << "\n";            
            LOG_DEBUG(s_s.str());
            s_s.clear();

            // s_s << "recv_data: \n";
            // for (KlineData& kline_data:append_result)
            // {
            //     result.emplace_back(boost::make_shared<KlineData>(kline_data));
            //     s_s << get_sec_time_str(kline_data.index) << " "
            //          << kline_data.px_open.get_value() << " "
            //          << kline_data.px_close.get_value() << " "
            //          << kline_data.px_high.get_value() << " "
            //          << kline_data.px_low.get_value() << " \n";
            // }
            // s_s << "\n";
            // LOG_DEBUG(s_s.str());
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

        // std::stringstream s_s;
        // s_s << "All Src Data \n";
        // for (KlineDataPtr& kline_data:result)
        // {
        //     s_s << get_sec_time_str(kline_data->index) << " "
        //             << kline_data->px_open.get_value() << " "
        //             << kline_data->px_close.get_value() << " "
        //             << kline_data->px_high.get_value() << " "
        //             << kline_data->px_low.get_value() << " \n";
        // }   
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_src_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

vector<KlineDataPtr> KlineProcess::compute_target_kline_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    try
    {
        vector<KlineDataPtr> result;

        KlineDataPtr last_data = get_last_kline_data(src_kline_data, frequency);

        result = compute_kline_atom_data(src_kline_data, frequency);

        result.push_back(last_data);

        return result;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::compute_target_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

KlineDataPtr KlineProcess::get_last_kline_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    try
    {
        KlineDataPtr last_data = src_kline_data[src_kline_data.size()-1];

        SDecimal low = last_data->px_low;
        SDecimal high = last_data->px_high;
        SDecimal open = last_data->px_open;

        src_kline_data.pop_back();

        while(src_kline_data.size() > 0 && last_data->index % frequency != 0)
        {
            KlineDataPtr atom = src_kline_data[src_kline_data.size()-1];
            low = low > atom->px_low ? atom->px_low:low;
            high = high < atom->px_high ? atom->px_high:high;        
            open = atom->px_open;
            last_data->index = atom->index;

            src_kline_data.pop_back();
        }

        last_data->px_low = low;
        last_data->px_high = high;
        last_data->px_open = open;

        return last_data;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_last_kline_data " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}

vector<KlineDataPtr> KlineProcess::compute_kline_atom_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    try
    {
        vector<KlineDataPtr> result;

        if (src_kline_data.size() == 0)
        {
            return result;
        }

        KlineDataPtr last_data = src_kline_data[src_kline_data.size()-1];
        result.push_back(last_data); 
        src_kline_data.pop_back();

        KlineDataPtr cur_data = boost::make_shared<KlineData>(*last_data);    

        for (int i = src_kline_data.size()-1; i >=0; --i)
        {
            KlineDataPtr atom = src_kline_data[i];
            type_tick last_time = (*result.rbegin())->index;

            // cur_data 已经遍历完当前的时间区间;
            // atom 作为下一个时间区间的 截止时间数据,开始进行遍历; 
            if (atom->index < last_time && last_time - atom->index >= frequency)
            {
                cur_data->index = cur_data->index - cur_data->index % frequency;

                KlineDataPtr data = boost::make_shared<KlineData>(*cur_data);

                result.push_back(data);

                cur_data = boost::make_shared<KlineData>(*atom);
            }
            else
            {
                cur_data->px_low = cur_data->px_low > atom->px_low ? atom->px_low:cur_data->px_low;
                cur_data->px_high = cur_data->px_high < atom->px_high ? atom->px_high:cur_data->px_high;
                cur_data->px_open = atom->px_open;
            }
        }

        // 最后末尾时间区间不够一个完整的 frequency；
        if (cur_data->index % frequency != 0)
        {
            cur_data->index = cur_data->index -  cur_data->index % frequency;
        }

        for (int i = 0; i < result.size()/2; ++i)
        {
            KlineDataPtr tmp = result[i];
            result[i] = result[result.size()-1-i];
            result[result.size()-1-i] = tmp;
        }

        return result;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::compute_kline_atom_data " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void KlineProcess::init_update_kline_data(PackagePtr rsp_package, ReqKLineDataPtr pReqKlineData)
{
    try
    {
        RspKLineDataPtr pRspKlineData = GetField<RspKLineData>(rsp_package);
        if (pRspKlineData)
        {
            auto iter = pRspKlineData->kline_data_vec_.rbegin();
            KlineDataUpdate kline_update(*pReqKlineData);

            kline_update.last_update_time_ = (*iter)->index;
            kline_update.kline_data_ = (*iter);

            std::stringstream s_s;
            s_s << "\nInit update_kline_data socket_id: " << pReqKlineData->socket_id_ << " " 
                << pReqKlineData->symbol_ << " Last Update Time: " 
                << get_sec_time_str(kline_update.last_update_time_) << "\n";
            LOG_DEBUG(s_s.str()); 

            updated_kline_data_map_[string(pReqKlineData->symbol_)].emplace_back(kline_update);          
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::init_update_kline_data " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    

}

bool KlineProcess::delete_kline_request_connect(string symbol, ID_TYPE socket_id)
{
    try
    {
        LOG_DEBUG(string("KlineProcess::delete_kline_request_connect ") + symbol + " " + std::to_string(socket_id));

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
    try
    {
        string symbol = kline_data->symbol;

        if (updated_kline_data_map_.find(symbol) != updated_kline_data_map_.end())
        {
            for (auto& kline_update:updated_kline_data_map_[symbol])
            {            
                int cur_fre = kline_update.reqkline_data.frequency_;

                if (kline_data->frequency_ > cur_fre) continue;

                if (!kline_update.kline_data_)
                {
                    kline_update.kline_data_ = boost::make_shared<KlineData>(*kline_data);
                }

                KlineDataPtr& last_kline = kline_update.kline_data_;

                if (last_kline->is_clear())
                {
                    last_kline->reset(*kline_data);

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

                    KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                    std::stringstream s_log;
                    s_log << "Kline Update New : " << get_sec_time_str(cur_kline_data->index) << " " 
                    << "fre: " << cur_kline_data->frequency_ << " "
                    <<"open: " << cur_kline_data->px_open.get_value() << " " 
                    <<"close: " << cur_kline_data->px_close.get_value() << " "
                    <<"high: " << cur_kline_data->px_high.get_value() << " "
                    <<"low: " << cur_kline_data->px_low.get_value() << " \n";
                    LOG_DEBUG(s_log.str());
                    
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

                    std::stringstream s_log;
                    s_log << "Kline Update old : " << get_sec_time_str(cur_kline_data->index) << " "
                    << "fre: " << cur_kline_data->frequency_ << " "
                    <<"open: " << cur_kline_data->px_open.get_value() << " " 
                    <<"close: " << cur_kline_data->px_close.get_value() << " "
                    <<"high: " << cur_kline_data->px_high.get_value() << " "
                    <<"low: " << cur_kline_data->px_low.get_value() << " \n";
                    LOG_DEBUG(s_log.str());

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
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::update_kline_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void KlineProcess::check_websocket_subinfo(ReqKLineDataPtr pReqKlineData)
{
    try
    {
        std::lock_guard<std::mutex> lk(wss_con_map_mutex_);
        if (wss_con_map_.find(pReqKlineData->socket_id_) != wss_con_map_.end())
        {
            delete_kline_request_connect(wss_con_map_[pReqKlineData->socket_id_], pReqKlineData->socket_id_);
        }
        wss_con_map_[pReqKlineData->socket_id_] = pReqKlineData->symbol_;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::check_websocket_subinfo: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    } 
}

void KlineProcess::update_frequency_aggreration_map(int src_fre)
{
    try
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
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::update_frequency_aggreration_map: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

int KlineProcess::get_best_freq_base(int req_frequency)
{
    try
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
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::get_best_freq_base: " << e.what() << "\n";
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
            if (pReqTradePtr->is_cancel_)
            {
                delete_trade_wss(pReqTradePtr);
            }
            else
            {
                LOG_DEBUG(pReqTradePtr->str());

                check_websocket_trade_req(pReqTradePtr);

                if (trade_data_map_.find(string(pReqTradePtr->symbol_)) != trade_data_map_.end())
                {
                    
                    PackagePtr package = get_trade_package(pReqTradePtr, trade_data_map_[string(pReqTradePtr->symbol_)]);
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
                    string error_msg =  "No Trade Data For " + string(pReqTradePtr->symbol_) ;                    
                    PackagePtr package = CreatePackage<RspErrorMsg>(error_msg, 1, pReqTradePtr->socket_id_, pReqTradePtr->socket_type_);
                    package->prepare_response(UT_FID_RspErrorMsg, ID_MANAGER->get_id());
                    LOG_ERROR(error_msg);
                }
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

void KlineProcess::response_src_trade_package(PackagePtr package)
{
    try
    {
        TradeDataPtr pTradeDataPtr = GetField<TradeData>(package);
        if (pTradeDataPtr)
        {
            update_trade_data(pTradeDataPtr);
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

void KlineProcess::check_websocket_trade_req(ReqTradePtr pReqTrade)
{
    try
    {
        string new_symbol = pReqTrade->symbol_;
        
        std::lock_guard<std::mutex> lk(trade_wss_con_map_mutex_);

        if (trade_wss_con_map_.find(pReqTrade->socket_id_) != trade_wss_con_map_.end() 
        && new_symbol != trade_wss_con_map_[pReqTrade->socket_id_])
        {
            string ori_symbol = trade_wss_con_map_[pReqTrade->socket_id_];

            std::lock_guard<std::mutex> lk(updated_trade_data_map_mutex_);
            if (updated_trade_data_map_.find(ori_symbol) != updated_trade_data_map_.end()
            && updated_trade_data_map_[ori_symbol].find(pReqTrade->socket_id_) != updated_trade_data_map_[ori_symbol].end())
            {
                updated_trade_data_map_[ori_symbol].erase(pReqTrade->socket_id_);

                LOG_DEBUG("ReqTrade updated_trade_data_map_  Erase " + ori_symbol + " socket: " + std::to_string(pReqTrade->socket_id_));
            }
        }         

        trade_wss_con_map_[pReqTrade->socket_id_] = new_symbol;

        TradeDataUpdatePtr pTradeDataUpdate = boost::make_shared<TradeDataUpdate>(*pReqTrade);

        std::lock_guard<std::mutex> lg(updated_trade_data_map_mutex_);
        updated_trade_data_map_[new_symbol][pReqTrade->socket_id_] = pTradeDataUpdate;         

        // cout << "ReqTrade trade_wss_con_map_ add  " << pReqTrade->symbol_ << " socket: " << pReqTrade->socket_id_ << endl;
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

void KlineProcess::delete_trade_wss(ReqTradePtr pReqTrade)
{
    try
    {
        std::lock_guard<std::mutex> lk(trade_wss_con_map_mutex_);
        if (trade_wss_con_map_.find(pReqTrade->socket_id_) != trade_wss_con_map_.end())
        {
            string ori_symbol = trade_wss_con_map_[pReqTrade->socket_id_];

            std::lock_guard<std::mutex> lk(updated_trade_data_map_mutex_);
            if (updated_trade_data_map_.find(ori_symbol) != updated_trade_data_map_.end()
            && updated_trade_data_map_[ori_symbol].find(pReqTrade->socket_id_) != updated_trade_data_map_[ori_symbol].end())
            {
                updated_trade_data_map_[ori_symbol].erase(pReqTrade->socket_id_);

                LOG_DEBUG( "ReqTrade Erase  " + ori_symbol + " socket: " + std::to_string(pReqTrade->socket_id_));
            }

            trade_wss_con_map_.erase(pReqTrade->socket_id_);
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] KlineProcess::delete_trade_wss " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] KlineProcess::delete_trade_wss: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }        
}

void KlineProcess::update_trade_data(TradeDataPtr pTradeDataPtr)
{
    try
    {
        string symbol = pTradeDataPtr->symbol_;
        {
            // std::lock_guard<std::mutex> lk(trade_data_map_mutex_);
            // TradeDataPtr oldTradeDataPtr;

            // if (trade_data_map_.find(symbol) == trade_data_map_.end())
            // {
            //     // cout << "KlineProcess::update_trade_data New Symbol: " << symbol << endl;
            //     oldTradeDataPtr = nullptr;
            // }
            // else
            // {
            //     oldTradeDataPtr = trade_data_map_[symbol];
            //     // cout <<"NewModeTime: " << get_sec_time_str(mod_secs(pTradeDataPtr->time_, trade_data_freq_base_)) 
            //     //      << " OldModeTime: " << get_sec_time_str(mod_secs(oldTradeDataPtr->time_, trade_data_freq_base_)) 
            //     //      << endl;
            // }
            // compute_trade_data(pTradeDataPtr, oldTradeDataPtr);

            update_new_trade(pTradeDataPtr);

            if (!(pTradeDataPtr->high_ == 0))
            {
                trade_data_map_[symbol] = pTradeDataPtr;
            }            
        }
        
        std::lock_guard<std::mutex> lk(updated_trade_data_map_mutex_);
        if (updated_trade_data_map_.find(symbol) != updated_trade_data_map_.end())
        {
            map<ID_TYPE, TradeDataUpdatePtr>& reqMap = updated_trade_data_map_[symbol];
            for (auto iter:reqMap)
            {
                PackagePtr package = get_trade_package(iter.second->pReqTrade_, trade_data_map_[symbol]);

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

void KlineProcess::compute_trade_data(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr)
{
    try
    {
        update_new_trade(curTradeDataPtr);       
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::compute_trade_data " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::compute_trade_data: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }        
}

bool KlineProcess::need_compute_new_trade(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr)
{
    try
    {
        if (oldTradeDataPtr == nullptr 
        || mod_secs(curTradeDataPtr->time_, trade_data_freq_base_) != mod_secs(oldTradeDataPtr->time_, trade_data_freq_base_))
        {
            if (oldTradeDataPtr)
            {
            }

            return true;
        }
        return false;
         
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::need_compute_new_trade " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::need_compute_new_trade: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }   
}

void KlineProcess::compute_new_trade(TradeDataPtr pTradeData)
{
    try
    {
        int end_time = pTradeData->time_;
        int start_time = end_time - 24 * 60 * 60;
        start_time = mod_secs(start_time, trade_data_freq_base_);

        std::vector<KlineDataPtr> src_kline_data = get_trade_kline_data(pTradeData->symbol_, trade_data_freq_base_, start_time, end_time);

        if (src_kline_data.size() > 0)
        {
            SDecimal start_price = src_kline_data[0]->index > start_time 
                                    ? src_kline_data[0]->px_open 
                                    : src_kline_data[0]->px_close;

            type_tick high_time;
            type_tick low_time;
            SDecimal high;
            SDecimal low;

            if (src_kline_data[0]->px_high > pTradeData->price_)
            {
                high = src_kline_data[0]->px_high;
                high_time = src_kline_data[0]->index;
            }
            else
            {
                high = pTradeData->price_;
                high_time = pTradeData->time_;
            }

            if (src_kline_data[0]->px_low < pTradeData->price_)
            {
                low = src_kline_data[0]->px_low;
                low_time = src_kline_data[0]->index;
            }
            else
            {
                low = pTradeData->price_;
                low_time = pTradeData->time_;
            }

            SDecimal price = pTradeData->price_;
            SDecimal volume = 0;
            double change = price.get_value() - start_price.get_value();
            double change_rate = change / start_price.get_value();

           
            for (KlineDataPtr& kline : src_kline_data)
            {
                // high = high < kline->px_high ? kline->px_high : high;
                // low = low > kline->px_low ? kline->px_low : low;
                volume += kline->volume;

                if (high < kline->px_high)
                {
                    high = kline->px_high;
                    high_time = kline->index;
                }

                if (low > kline->px_low)
                {
                    low = kline->px_low;
                    low_time = kline->index;
                }               
            }

            pTradeData->start_time_ = src_kline_data[0]->index;
            pTradeData->start_price_ = start_price;
            pTradeData->total_volume_ = volume;
            pTradeData->high_ = high;
            pTradeData->low_ = low;            
            pTradeData->change_ = change;
            pTradeData->change_rate_ = change_rate;
        }
        else
        {
            LOG_WARN(string("KlineProcess::compute_new_trade Cann't Get Src Data ") + pTradeData->symbol_);
        }        
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::compute_new_trade " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::compute_new_trade: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }  
}

void KlineProcess::update_new_trade(TradeDataPtr curTradeDataPtr)
{
    try
    {
        string symbol = curTradeDataPtr->symbol_;

        if (one_day_kline_data_.find(symbol) == one_day_kline_data_.end() || one_day_kline_data_[symbol].is_empty())
        {
            curTradeDataPtr->high_ = 0;
            curTradeDataPtr->low_ = 0;
        }
        else
        {
            TimeKlineData& cur_time_data = one_day_kline_data_[string(curTradeDataPtr->symbol_)];
            curTradeDataPtr->high_ = curTradeDataPtr->price_ > cur_time_data.high_ ? curTradeDataPtr->price_: cur_time_data.high_;
            curTradeDataPtr->low_ = curTradeDataPtr->price_ < cur_time_data.low_ ? curTradeDataPtr->price_ : cur_time_data.low_;

            type_tick high_time = curTradeDataPtr->high_ == curTradeDataPtr->price_? curTradeDataPtr->time_ : cur_time_data.high_time_;
            type_tick low_time = curTradeDataPtr->low_ == curTradeDataPtr->price_ ? curTradeDataPtr->time_ : cur_time_data.low_time_;

            curTradeDataPtr->change_ = curTradeDataPtr->price_.get_value() - cur_time_data.start_price_.get_value();
            curTradeDataPtr->change_rate_ = curTradeDataPtr->change_ / cur_time_data.start_price_.get_value();   
        }

    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::update_new_trade " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::update_new_trade: unkonwn exception! " << "\n";
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

            int end_time = pTradeData->time_;
            int start_time = end_time - 24 * 60 * 60;
            start_time = mod_secs(start_time, trade_data_freq_base_);

            std::vector<KlineDataPtr> src_kline_data = get_trade_kline_data(symbol, trade_data_freq_base_, start_time, end_time);

            if (src_kline_data.size() > 0)
            {
                SDecimal start_price = src_kline_data[0]->index > start_time 
                                        ? src_kline_data[0]->px_open 
                                        : src_kline_data[0]->px_close;

                SDecimal high = src_kline_data[0]->px_high > pTradeData->price_ ? src_kline_data[0]->px_high : pTradeData->price_;
                SDecimal low = src_kline_data[0]->px_low < pTradeData->price_?src_kline_data[0]->px_low : pTradeData->price_;
                SDecimal price = pTradeData->price_;
                SDecimal volume = 0;
                double change = price.get_value() - start_price.get_value();
                double change_rate = change / start_price.get_value();

                for (KlineDataPtr& kline : src_kline_data)
                {
                    high = high < kline->px_high ? kline->px_high : high;
                    low = low > kline->px_low ? kline->px_low : low;
                    volume += kline->volume;
                }

                result = CreatePackage<RspTrade>(pReqTrade->symbol_, price, volume, 
                                                 change, change_rate, high, low, 
                                                 pReqTrade->socket_id_, pReqTrade->socket_type_);
                if (!result)
                {
                    LOG_ERROR("KlineProcess::get_trade_package CreatePackage<RspTrade> Failed!");
                }
                else
                {
                    result->prepare_response(UT_FID_RspTrade, ID_MANAGER->get_id());
                }
            }
            else
            {
                cout << "KlineProcess::get_trade_package Cann't Get Src Data " << pReqTrade->symbol_ << endl;
            }
        }
        else
        {
            string error_msg =  "No Trade Data For " + string(pReqTrade->symbol_) ;
            
            result = CreatePackage<RspErrorMsg>(error_msg, 1, pReqTrade->socket_id_, pReqTrade->socket_type_);
            result->prepare_response(UT_FID_RspErrorMsg, ID_MANAGER->get_id());
            LOG_ERROR(error_msg);
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

PackagePtr KlineProcess::get_trade_package(ReqTradePtr pReqTrade, TradeDataPtr pTradeDataPtr)
{
    try
    {
        PackagePtr result = CreatePackage<RspTrade>(pReqTrade->symbol_, pTradeDataPtr->price_, pTradeDataPtr->total_volume_, 
                                                    pTradeDataPtr->change_, pTradeDataPtr->change_rate_, 
                                                    pTradeDataPtr->high_, pTradeDataPtr->low_, 
                                                    pReqTrade->socket_id_, pReqTrade->socket_type_);
        
        if (!result)
        {
            LOG_ERROR("KlineProcess::get_trade_package CreatePackage<RspTrade> Failed!");
        }
        else
        {
            result->prepare_response(UT_FID_RspTrade, ID_MANAGER->get_id());            
        }       

        return result; 

    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "KlineProcess::get_trade_package " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "KlineProcess::get_trade_package: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }  
}

std::vector<KlineDataPtr> KlineProcess::get_trade_kline_data(string symbol,  int freq_base, int start_time, int end_time)
{
    try
    {
         std::vector<KlineDataPtr> result;
        bool need_more_data = false;
        std::map<type_tick, KlineDataPtr> cur_src_data;
        int request_end_time = end_time;

        if (kline_data_.find(symbol) == kline_data_.end() || 
            kline_data_[symbol].find(freq_base) ==  kline_data_[symbol].end())
        {
            need_more_data = true;      
        }
        else if (kline_data_[symbol][freq_base].begin()->first > start_time)
        {
            request_end_time = kline_data_[symbol][freq_base].begin()->first;
            need_more_data = true;
            cur_src_data = kline_data_[symbol][freq_base];
        }
        else
        {
            cur_src_data = kline_data_[symbol][freq_base];
        }

        if (need_more_data)
        {
            vector<KlineData> append_result;
            HubInterface::get_kline("", symbol.c_str(), freq_base, start_time, request_end_time, append_result);


            for (KlineData& kline_data:append_result)
            {
                result.emplace_back(boost::make_shared<KlineData>(kline_data));
            }
        }
        else
        {

        }

        if (cur_src_data.size() > 0)
        {
            // cout << "\nCached Data: " <<endl;
            std::map<type_tick, KlineDataPtr>::iterator iter = cur_src_data.begin();
            while (iter != cur_src_data.end() && iter->first < start_time)
            {
                ++iter;
            }

            while(iter != cur_src_data.end() && iter->first <= end_time ) 
            {
                result.push_back(iter->second);     
                ++iter;
            }
        }
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

// vector<KlineDataPtr> KlineProcess::compute_target_kline_data_bak(vector<KlineDataPtr>& src_kline_data, int frequency)
// {
//     vector<KlineDataPtr> result;

//     cout << "compute_target_kline_data" << endl;

//     if (src_kline_data.size() == 0)
//     {
//         return result;
//     }

//     KlineDataPtr cur_data = src_kline_data[0];
//     // cout << "\ncur_data.tick: " << cur_data->tick_ << endl;

//     result.push_back(cur_data); 

//     SDecimal low(MAX_DOUBLE);
//     SDecimal high(MIN_DOUBLE);
//     SDecimal open;

//     // cout << "kline_data.size: " << src_kline_data.size() << endl;

//     src_kline_data.erase(src_kline_data.begin());

//     bool is_first = true;
//     for (KlineDataPtr atom:src_kline_data)
//     {
//         low = low > atom->px_low ? atom->px_low:low;
//         high = high < atom->px_high ? atom->px_high:high;

//         if (is_first)
//         {
//             open = atom->px_open;
//             is_first = false;
//         }
        
//         if (atom->index > (*result.rbegin())->index && atom->index - (*result.rbegin())->index >= frequency)
//         {            
//             KlineDataPtr cur_data = boost::make_shared<KlineData>(*atom);
//             cur_data->px_low = low;
//             cur_data->px_high = high;
//             cur_data->px_open = open;

//             is_first = true;

//             // cout << "cur_data: " << get_sec_time_str(atom->index) << " "
//             //      << "last_data: " << get_sec_time_str((*result.rbegin())->index) << " "
//             //      << "delta: " << atom->index - (*result.rbegin())->index << " "
//             //      << "fre: " << frequency << endl;

//             result.push_back(cur_data); 

//             low = MAX_DOUBLE;
//             high = MIN_DOUBLE;
//         }
//     }
//     return result;
// }


// vector<KlineDataPtr> KlineProcess::compute_day_kline_data(vector<KlineDataPtr>& src_kline_data, int frequency)
// {
//     try
//     {
//     vector<KlineDataPtr> result;

//     KlineDataPtr last_data = src_kline_data[src_kline_data.size()-1];
//     // cout << "\ncur_data.tick: " << cur_data->tick_ << endl;

//     SDecimal low = last_data->px_low;
//     SDecimal high = last_data->px_high;
//     SDecimal open = last_data->px_open;

//     // cout << "kline_data.size: " << src_kline_data.size() << endl;

//     src_kline_data.pop_back();

//     int day_secs = 24 * 3600;

//     while(src_kline_data.size() > 0)
//     {
//         KlineDataPtr atom = src_kline_data[src_kline_data.size()-1];
//         low = low > atom->px_low ? atom->px_low:low;
//         high = high < atom->px_high ? atom->px_high:high;        
//         open = atom->px_open;

//         if (src_kline_data[src_kline_data.size()-1]->index % frequency == 0)
//         {
//             src_kline_data.pop_back();
//             break;
//         }
//         src_kline_data.pop_back();
//     }

//     last_data->px_low = low;
//     last_data->px_high = high;
//     last_data->px_open = open;

//     last_data->index = last_data->index -  last_data->index % frequency;

//     result = compute_kline_atom_data(src_kline_data, frequency);

//     result.push_back(last_data);

//     return result;

//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
//     }
    
// }