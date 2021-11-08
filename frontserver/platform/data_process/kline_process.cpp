#include "../front_server_declare.h"
#include "../config/config.h"
#include "../log/log.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../util/id.hpp"

#include "kline_process.h"
#include "hub_interface.h"
#include "data_process.h"


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

                if (curr_time - last_time >= frequency_ && ori_data_.find(curr_time) == ori_data_.end())
                {
                    ori_data_[curr_time] = kline_data;

                    if (curr_time - first_time >= last_secs_)
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

                if ("BTC_USDT" == string(kline_data->symbol))
                {
                    LOG_DEBUG("high: " + high_.get_str_value() + ", ht: " + get_sec_time_str(high_time_) 
                            + ", low: " + low_.get_str_value() + ", lt: " + get_sec_time_str(low_time_));
                }
            }

            start_price_ = ori_data_.begin()->second->px_open;            
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
                delete_sub_kline(pReqKlineData->symbol_, pReqKlineData->frequency_);
            }
            else
            {                
                PackagePtr rsp_package = get_request_kline_package(package);

                if (rsp_package)
                {
                    if (rsp_package->PackageID() == UT_FID_RspKLineData)
                    {
                        init_subed_update_kline_data(rsp_package, pReqKlineData);
                    }

                    process_engine_->deliver_response(rsp_package);
                }
                else
                {
                    LOG_ERROR("get_request_kline_package failed!");
                }     
            }       
        }
        else
        {
            LOG_ERROR("ReqKLineData NULL!");
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("Unknow Exceptions!");        
    }
}

void KlineProcess::response_kline_package(PackagePtr package)
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
            update_oneday_kline_data(update_for_trade);

            KlineDataPtr update_for_kline = boost::make_shared<KlineData>(*pkline_data);
            update_subed_kline_data(update_for_kline);

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
            LOG_ERROR("pkline_data is NULL!");
        }        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void KlineProcess::update_oneday_kline_data(const KlineDataPtr kline_data)
{
    try
    {
        string cur_symbol = string(kline_data->symbol);

        if (oneday_updated_kline_data_.find(cur_symbol) == oneday_updated_kline_data_.end())
        {
            oneday_updated_kline_data_[cur_symbol] = TimeKlineData(60, 60*60*24, cur_symbol, this);
        }

        oneday_updated_kline_data_[cur_symbol].update(kline_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void KlineProcess::update_subed_kline_data(const KlineDataPtr kline_data)
{
    try
    {
        string symbol = kline_data->symbol;

        if (sub_updated_kline_map_.find(symbol) != sub_updated_kline_map_.end())
        {
            map<int, KlineDataPtr>& cur_symbol_sub_map = sub_updated_kline_map_[symbol];

            for (auto iter:cur_symbol_sub_map)
            {
                int cur_fre = iter.first;
                KlineDataPtr& last_kline = iter.second;

                if (cur_fre < kline_data->frequency_)
                {
                    continue;
                }
                
                if (last_kline->is_clear())
                {
                    assign(last_kline->px_open, kline_data->px_open);
                    assign(last_kline->px_close, kline_data->px_close);
                    assign(last_kline->px_high, kline_data->px_high);
                    assign(last_kline->px_low, kline_data->px_low);
                    assign(last_kline->volume, kline_data->volume);

                    last_kline->clear_ = false; 
                }
                else
                {
                    last_kline->px_close = kline_data->px_close;
                    last_kline->px_low = last_kline->px_low > kline_data->px_low ? kline_data->px_low: last_kline->px_low;
                    last_kline->px_high = last_kline->px_high < kline_data->px_high ? kline_data->px_high: last_kline->px_high;
                    assign(last_kline->symbol, kline_data->symbol);                
                }



                if (kline_data->index - last_kline->index >= last_kline->frequency_)
                {
                    // 推送更新作为下一个 k 线时间数据;

                    last_kline->index = kline_data->index;
                    KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

                    std::stringstream s_log;
                    s_log << "New : " << cur_kline_data->symbol << " "
                          << get_sec_time_str(cur_kline_data->index) << " " 
                          << "fre: " << kline_data->frequency_ << " "
                          << "open: " << cur_kline_data->px_open.get_value() << " " 
                          << "close: " << cur_kline_data->px_close.get_value() << " "
                          << "high: " << cur_kline_data->px_high.get_value() << " "
                          << "low: " << cur_kline_data->px_low.get_value() << " \n";
                    LOG_DEBUG(s_log.str());

                    PackagePtr rsp_package = CreatePackage<RspKLineData>(cur_kline_data);
                    if (rsp_package)
                    {
                        rsp_package->prepare_response(UT_FID_RspKLineData, ID_MANAGER->get_id());
                        process_engine_->deliver_response(rsp_package);
                    }  
                    else
                    {
                        LOG_ERROR("Create RspKLineData Package Failed!");
                    }
                    last_kline->clear();
                }
                else
                {
                    // 推送更新作为当前 k 线时间数据;
                    KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);
                    // cur_kline_data->index = kline_update.last_update_time_;

                    std::stringstream s_log;
                    s_log << "old : " << cur_kline_data->symbol << " "
                          << get_sec_time_str(cur_kline_data->index) << " "
                          << "fre: " << kline_data->frequency_ << " "
                          << "open: " << cur_kline_data->px_open.get_value() << " " 
                          << "close: " << cur_kline_data->px_close.get_value() << " "
                          << "high: " << cur_kline_data->px_high.get_value() << " "
                          << "low: " << cur_kline_data->px_low.get_value() << " \n";
                    LOG_DEBUG(s_log.str());

                    PackagePtr rsp_package = CreatePackage<RspKLineData>(cur_kline_data);
                    if (rsp_package)
                    {
                        rsp_package->prepare_response(UT_FID_RspKLineData, ID_MANAGER->get_id());

                        process_engine_->deliver_response(rsp_package);
                    }  
                    else
                    {
                        LOG_ERROR("CreatePackage<RspKLineData> Failed!");
                    }

                }

            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
                }
                else
                {
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
        LOG_ERROR(e.what());
    }

    return false;
}

PackagePtr KlineProcess::get_request_kline_package(PackagePtr package)
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

            PackagePtr rsp_package = nullptr;

            if (best_freq_base == -1)
            {
                stringstream s_obj;
                s_obj << "No KLine Data For " << pReqKlineData->symbol_ 
                      << " fre: " << pReqKlineData->frequency_ 
                      << " data_count: " << pReqKlineData->data_count_;
                LOG_WARN(s_obj.str());

                string err_msg = s_obj.str();
                int err_id = 1;
                rsp_package = GetRspErrMsgPackage(err_msg, err_id, pReqKlineData->socket_id_, pReqKlineData->socket_type_);
                return rsp_package; 
            }
                        
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
                    
                    HubInterface::get_kline("", pReqKlineData->symbol_, 
                                            best_freq_base, pReqKlineData->start_time_, 
                                            symbol_kline_data.begin()->first, append_result);
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
                s_obj << "Needed Full SrcData Count: " << data_count << "\n";
                get_src_kline_data(pReqKlineData->symbol_, src_kline_data, symbol_kline_data, data_count, best_freq_base);
            }            

            s_obj << "Src Kline Data Size: " << src_kline_data.size() << "\n";

            // for (auto kline_data:src_kline_data)
            // {
            //     s_obj << kline_data->symbol << " " << get_sec_time_str(kline_data->index) << " "
            //         << kline_data->px_open.get_value() << " "
            //         << kline_data->px_close.get_value() << " "
            //         << kline_data->px_high.get_value() << " "
            //         << kline_data->px_low.get_value() << " \n";
            // }

            vector<KlineDataPtr> target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);
            s_obj << "Rsp Kline Data Size: " << target_kline_data.size() << "\n";

            if (target_kline_data.size() > 0)
            {
                // for (auto kline_data:target_kline_data)
                // {
                //     s_obj << kline_data->symbol << " " << get_sec_time_str(kline_data->index) << " "
                //         << kline_data->px_open.get_value() << " "
                //         << kline_data->px_close.get_value() << " "
                //         << kline_data->px_high.get_value() << " "
                //         << kline_data->px_low.get_value() << " \n";
                // }

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
                LOG_WARN(s_obj.str());

                string err_msg = s_obj.str();
                int err_id = 1;
                rsp_package = GetRspErrMsgPackage(err_msg, err_id, pReqKlineData->socket_id_, pReqKlineData->socket_type_);
            }
            
            return rsp_package;
        }
        else
        {
            LOG_ERROR(string("Symbol: ") + string(pReqKlineData->symbol_) + string(" does not exit!"));       
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return nullptr;
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
        LOG_ERROR(e.what());
    }
}

vector<KlineDataPtr> KlineProcess::compute_target_kline_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    vector<KlineDataPtr> result;

    try
    {
        KlineDataPtr last_data = get_last_kline_data(src_kline_data, frequency);

        result = compute_kline_atom_data(src_kline_data, frequency);

        result.push_back(last_data);

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return result;
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
        LOG_ERROR(e.what());
    }

    return nullptr;
}

vector<KlineDataPtr> KlineProcess::compute_kline_atom_data(vector<KlineDataPtr>& src_kline_data, int frequency)
{
    vector<KlineDataPtr> result;

    try
    {        

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

        // 最后末尾时间区间不够一个完整的 frequency, 暂时放弃;
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
        LOG_ERROR(e.what());
    }

    return result;
}

void KlineProcess::init_subed_update_kline_data(PackagePtr rsp_package, ReqKLineDataPtr pReqKlineData)
{
    try
    {
        RspKLineDataPtr pRspKlineData = GetField<RspKLineData>(rsp_package);
        if (pRspKlineData)
        {
            auto iter = pRspKlineData->kline_data_vec_.rbegin();

            if (pRspKlineData->kline_data_vec_.size() > 0)
            {
                string symbol = pReqKlineData->symbol_;
                int freq = pReqKlineData->frequency_;
                if (sub_updated_kline_map_.find(symbol) == sub_updated_kline_map_.end()
                || sub_updated_kline_map_[symbol].find(freq) != sub_updated_kline_map_[symbol].end())
                {
                    sub_updated_kline_map_[symbol][freq] = pRspKlineData->kline_data_vec_[pRspKlineData->kline_data_vec_.size()-1];
                }
            }
            else
            {
                LOG_ERROR("pRspKlineData->kline_data_vec_ is Empty!");
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool KlineProcess::delete_sub_kline(string symbol, int freq)
{
    try
    {
        if (sub_updated_kline_map_.find(symbol) != sub_updated_kline_map_.end()
        && sub_updated_kline_map_[symbol].find(freq) != sub_updated_kline_map_[symbol].end())
        {
            sub_updated_kline_map_[symbol].erase(freq);
            return true;
        }
        return false;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    

    return false;
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
        int result = -1;
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
        LOG_ERROR(e.what());
    }
    return -1;
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
                delete_sub_trade(pReqTradePtr->symbol_);
            }
            else
            {
                LOG_INFO(pReqTradePtr->str());

                if (trade_data_map_.find(string(pReqTradePtr->symbol_)) != trade_data_map_.end())
                {                    
                    TradeDataPtr& pTradeDataPtr = trade_data_map_[string(pReqTradePtr->symbol_)];
                    PackagePtr rsp_package = CreatePackage<RspTrade>(pReqTradePtr->symbol_, pTradeDataPtr->price_, pTradeDataPtr->total_volume_, 
                                                                pTradeDataPtr->change_, pTradeDataPtr->change_rate_, 
                                                                pTradeDataPtr->high_, pTradeDataPtr->low_);
                    {
                        std::lock_guard<std::mutex> lk(sub_updated_trade_set_mutex_);
                        if (sub_updated_trade_set_.find(string(pReqTradePtr->symbol_)) == sub_updated_trade_set_.end())
                        {
                            sub_updated_trade_set_.emplace(string(pReqTradePtr->symbol_));
                        }
                    }

                    if (rsp_package)
                    {
                        RspTradePtr pRspTradeData = GetField<RspTrade>(rsp_package);

                        // if (pRspTradeData)
                        // {
                        //     LOG_INFO(pRspTradeData->get_json_str());     
                        // }                 
                        // else
                        // {
                        //     LOG_ERROR("GetField<RspTrade>(rsp_package) Failed");
                        // }
                        
                        rsp_package->prepare_response(UT_FID_RspTrade, ID_MANAGER->get_id());
                        process_engine_->deliver_response(rsp_package);
                    }
                    else
                    {
                        LOG_ERROR("CreatePackage<RspTrade> Failed!");
                    }                    
                }
                else
                {
                    string error_msg =  "No Trade Data For " + string(pReqTradePtr->symbol_) ;        
                    LOG_ERROR(error_msg);

                    PackagePtr rsp_package = CreatePackage<RspErrorMsg>(error_msg, 1, pReqTradePtr->socket_id_, pReqTradePtr->socket_type_);
                    if (rsp_package)
                    {
                        rsp_package->prepare_response(UT_FID_RspErrorMsg, ID_MANAGER->get_id());
                        process_engine_->deliver_response(rsp_package);
                    }
                    else
                    {
                        LOG_ERROR("CreatePackage<RspErrorMsg> Failed");
                    }
                    
                }
            }
        }
        else
        {
            LOG_ERROR("pReqTradePtr is NULL!");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("unkonwn exception!" );
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

            std::lock_guard<std::mutex> lk(sub_updated_trade_set_mutex_);

            if (sub_updated_trade_set_.find(pTradeDataPtr->symbol_) != sub_updated_trade_set_.end())
            {
                PackagePtr package = CreatePackage<RspTrade>(pTradeDataPtr->symbol_, pTradeDataPtr->price_, 
                                                             pTradeDataPtr->total_volume_, 
                                                             pTradeDataPtr->change_, pTradeDataPtr->change_rate_, 
                                                             pTradeDataPtr->high_, pTradeDataPtr->low_);

                // {
                //     type_tick high_time = oneday_updated_kline_data_[pTradeDataPtr->symbol_].high_time_;
                //     type_tick low_time = oneday_updated_kline_data_[pTradeDataPtr->symbol_].low_time_;

                //     std::stringstream stream_obj;
                //     stream_obj << "[TradeUp] " << pTradeDataPtr->symbol_ 
                //                << ", " << pTradeDataPtr->price_.get_value() 
                //                << ", " << pTradeDataPtr->total_volume_.get_value()
                //                << ",ht: " << get_sec_time_str(high_time)
                //                << ", " << pTradeDataPtr->high_.get_value()
                //                << ",lt: " << get_sec_time_str(low_time)
                //                << ", " << pTradeDataPtr->low_.get_value()
                //                << ", " << pTradeDataPtr->change_
                //                << ", " << pTradeDataPtr->change_rate_;
                //     LOG_DEBUG(stream_obj.str());                         
                // }

                if (package)
                {
                    package->prepare_response(UT_FID_RspTrade, ID_MANAGER->get_id());
                    process_engine_->deliver_response(package);
                }
                else
                {
                    LOG_ERROR("CreatePackage RspTrade Failed!");
                }
            } 
        }
        else 
        {
            LOG_ERROR("GetField Failed!");
        }        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("Unkonwn Exception!");
    }    
}

void KlineProcess::delete_sub_trade(string symbol)
{
    try
    {
        std::lock_guard<std::mutex> lk(sub_updated_trade_set_mutex_);

        if (sub_updated_trade_set_.find(symbol) != sub_updated_trade_set_.end())
        {
            sub_updated_trade_set_.erase(symbol);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("Unkonwn exception! ");
    }        
}

void KlineProcess::update_trade_data(TradeDataPtr curTradeDataPtr)
{
    try
    {
        string symbol = curTradeDataPtr->symbol_;

        if (oneday_updated_kline_data_.find(symbol) == oneday_updated_kline_data_.end() 
        || oneday_updated_kline_data_[symbol].is_empty())
        {
            curTradeDataPtr->high_ = 0;
            curTradeDataPtr->low_ = 0;
        }
        else
        {
            TimeKlineData& cur_time_data = oneday_updated_kline_data_[string(curTradeDataPtr->symbol_)];
            curTradeDataPtr->high_ = curTradeDataPtr->price_ > cur_time_data.high_ ? curTradeDataPtr->price_: cur_time_data.high_;
            curTradeDataPtr->low_ = curTradeDataPtr->price_ < cur_time_data.low_ ? curTradeDataPtr->price_ : cur_time_data.low_;

            type_tick high_time = curTradeDataPtr->high_ == curTradeDataPtr->price_ ? curTradeDataPtr->time_ : cur_time_data.high_time_;
            type_tick low_time = curTradeDataPtr->low_ == curTradeDataPtr->price_ ? curTradeDataPtr->time_ : cur_time_data.low_time_;

            curTradeDataPtr->change_ = curTradeDataPtr->price_.get_value() - cur_time_data.start_price_.get_value();
            curTradeDataPtr->change_rate_ = curTradeDataPtr->change_ / cur_time_data.start_price_.get_value();   

            trade_data_map_[symbol] = curTradeDataPtr;

            if ("BTC_USDT" == string(symbol))
            {
                LOG_DEBUG("\ntrade.high: " + curTradeDataPtr->high_.get_str_value() + ", trade.ht: " + get_sec_time_str(high_time)
                + ", trade.low: " + curTradeDataPtr->low_.get_str_value() + ", trade.lt: " + get_sec_time_str(low_time)
                + "\nkline.high: " + cur_time_data.high_.get_str_value()+ ", kline.ht: " + get_sec_time_str(cur_time_data.high_time_)
                + ", kline.low: " + cur_time_data.low_.get_str_value()+ ", kline.lt: " + get_sec_time_str(cur_time_data.low_time_)
                + "\ncur_price: " + curTradeDataPtr->price_.get_str_value() + ", ct: " + get_sec_time_str(curTradeDataPtr->time_)
                + ", start_price: " + cur_time_data.start_price_.get_str_value() + ", st: " + get_sec_time_str(cur_time_data.ori_data_.begin()->first));
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    catch(...)
    {
        LOG_ERROR("Unkonwn exception!");
    }    
}


// void init(type_tick end_time);

// void complete(type_tick end_time);

// void TimeKlineData::init(type_tick end_time)
// {
//     try
//     {
//         int end_time;
//         int start_time = end_time - 24 * 60 * 60;
//         start_time = mod_secs(start_time, frequency_);

//         std::vector<KlineDataPtr> src_kline_data = kline_process_->get_trade_kline_data(symbol_, frequency_, start_time, end_time);

//         if (src_kline_data.size() > 0)
//         {
//             start_price_ = src_kline_data[0]->px_open;

//             high_ = src_kline_data[0]->px_high;
//             high_time_ = src_kline_data[0]->index;
//             low_ = src_kline_data[0]->px_low;
//             low_time_ = src_kline_data[0]->index;

//             for (auto kline_data:src_kline_data)
//             {
//                 if (high_ < kline_data->px_high)
//                 {
//                     high_ = kline_data->px_high;
//                     high_time_ = kline_data->index;
//                 }

//                 if (low_ > kline_data->px_low)
//                 {
//                     low_ = kline_data->px_low;
//                     low_time_ = kline_data->index;
//                 }
//             }

//             std::stringstream s_s;

//             s_s << symbol_ << " "
//                 << "start_time: " << get_sec_time_str(src_kline_data[0]->index) << " \n"
//                 << "end_time: " << get_sec_time_str(src_kline_data[src_kline_data.size()-1]->index) << "\n"
//                 << "high_time: " << get_sec_time_str(high_time_) << " high: " << high_.get_value() << "\n"
//                 << "low_time: " << get_sec_time_str(low_time_)   << " low: " << low_.get_value() << "\n"
//                 << "kline_data.size: " << src_kline_data.size() << " \n";

//             LOG_DEBUG(s_s.str());
//         }
//         else
//         {
//             LOG_ERROR( "Get No Kline Source Data");
//         }
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
    
// }

// void TimeKlineData::complete(type_tick end_time)
// {
//     try
//     {
//         if (ori_data_.size() == 0)
//         {
//             init(end_time);
//         }
//         else
//         {
//             type_tick first_time = ori_data_.begin()->first;
//             type_tick last_time = ori_data_.rbegin()->first;

//             if (last_time - first_time < last_secs_)
//             {
//                 type_tick req_start_time = last_time - last_secs_;
//                 type_tick req_end_time = first_time - frequency_;;;

//                 std::vector<KlineDataPtr> src_kline_data = kline_process_->get_trade_kline_data(symbol_, frequency_, req_start_time, req_end_time);

//                 for (auto kline_data:src_kline_data)
//                 {
//                     ori_data_[kline_data->index] = kline_data;
//                 }

//                 refresh_high_low();

//                 std::stringstream s_s;
//                 s_s << symbol_ << " "
//                     << "start_time: " << get_sec_time_str(src_kline_data[0]->index) << " \n"
//                     << "end_time: " << get_sec_time_str(src_kline_data[src_kline_data.size()-1]->index) << "\n"
//                     << "high_time: " << get_sec_time_str(high_time_) << " high: " << high_.get_value() << "\n"
//                     << "low_time: " << get_sec_time_str(low_time_)   << " low: " << low_.get_value() << "\n"
//                     << "kline_data.size: " << src_kline_data.size() << " \n";
//                 LOG_DEBUG(s_s.str());
//             }            
//         }        
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
// }


// bool KlineProcess::need_compute_new_trade(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr)
// {
//     try
//     {
//         if (oldTradeDataPtr == nullptr 
//         || mod_secs(curTradeDataPtr->time_, trade_data_freq_base_) != mod_secs(oldTradeDataPtr->time_, trade_data_freq_base_))
//         {
//             if (oldTradeDataPtr)
//             {
//             }

//             return true;
//         }
//         return false;
         
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("unkonwn exception!");
//     }   
// }

// void KlineProcess::compute_new_trade(TradeDataPtr pTradeData)
// {
//     try
//     {
//         int end_time = pTradeData->time_;
//         int start_time = end_time - 24 * 60 * 60;
//         start_time = mod_secs(start_time, trade_data_freq_base_);

//         std::vector<KlineDataPtr> src_kline_data = get_trade_kline_data(pTradeData->symbol_, trade_data_freq_base_, start_time, end_time);

//         if (src_kline_data.size() > 0)
//         {
//             SDecimal start_price = src_kline_data[0]->index > start_time 
//                                     ? src_kline_data[0]->px_open 
//                                     : src_kline_data[0]->px_close;

//             type_tick high_time;
//             type_tick low_time;
//             SDecimal high;
//             SDecimal low;

//             if (src_kline_data[0]->px_high > pTradeData->price_)
//             {
//                 high = src_kline_data[0]->px_high;
//                 high_time = src_kline_data[0]->index;
//             }
//             else
//             {
//                 high = pTradeData->price_;
//                 high_time = pTradeData->time_;
//             }

//             if (src_kline_data[0]->px_low < pTradeData->price_)
//             {
//                 low = src_kline_data[0]->px_low;
//                 low_time = src_kline_data[0]->index;
//             }
//             else
//             {
//                 low = pTradeData->price_;
//                 low_time = pTradeData->time_;
//             }

//             SDecimal price = pTradeData->price_;
//             SDecimal volume = 0;
//             double change = price.get_value() - start_price.get_value();
//             double change_rate = change / start_price.get_value();

           
//             for (KlineDataPtr& kline : src_kline_data)
//             {
//                 // high = high < kline->px_high ? kline->px_high : high;
//                 // low = low > kline->px_low ? kline->px_low : low;
//                 volume += kline->volume;

//                 if (high < kline->px_high)
//                 {
//                     high = kline->px_high;
//                     high_time = kline->index;
//                 }

//                 if (low > kline->px_low)
//                 {
//                     low = kline->px_low;
//                     low_time = kline->index;
//                 }               
//             }

//             pTradeData->start_time_ = src_kline_data[0]->index;
//             pTradeData->start_price_ = start_price;
//             pTradeData->total_volume_ = volume;
//             pTradeData->high_ = high;
//             pTradeData->low_ = low;            
//             pTradeData->change_ = change;
//             pTradeData->change_rate_ = change_rate;
//         }
//         else
//         {
//             LOG_WARN(string("Cann't Get Src Data ") + pTradeData->symbol_);
//         }        
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("unkonwn exception!");
//     }  
// }

// PackagePtr KlineProcess::get_trade_package(ReqTradePtr pReqTrade)
// {
//     try
//     {
//         PackagePtr result = nullptr;
//         string symbol = pReqTrade->symbol_;
//         if (trade_data_map_.find(symbol)!= trade_data_map_.end())
//         {
//             TradeDataPtr pTradeData = trade_data_map_[symbol];

//             int end_time = pTradeData->time_;
//             int start_time = end_time - 24 * 60 * 60;
//             start_time = mod_secs(start_time, trade_data_freq_base_);

//             std::vector<KlineDataPtr> src_kline_data = get_trade_kline_data(symbol, trade_data_freq_base_, start_time, end_time);

//             if (src_kline_data.size() > 0)
//             {
//                 SDecimal start_price = src_kline_data[0]->index > start_time 
//                                         ? src_kline_data[0]->px_open 
//                                         : src_kline_data[0]->px_close;

//                 SDecimal high = src_kline_data[0]->px_high > pTradeData->price_ ? src_kline_data[0]->px_high : pTradeData->price_;
//                 SDecimal low = src_kline_data[0]->px_low < pTradeData->price_?src_kline_data[0]->px_low : pTradeData->price_;
//                 SDecimal price = pTradeData->price_;
//                 SDecimal volume = 0;
//                 double change = price.get_value() - start_price.get_value();
//                 double change_rate = change / start_price.get_value();

//                 for (KlineDataPtr& kline : src_kline_data)
//                 {
//                     high = high < kline->px_high ? kline->px_high : high;
//                     low = low > kline->px_low ? kline->px_low : low;
//                     volume += kline->volume;
//                 }

//                 result = CreatePackage<RspTrade>(pReqTrade->symbol_, price, volume, 
//                                                  change, change_rate, high, low, 
//                                                  pReqTrade->socket_id_, pReqTrade->socket_type_);
//                 if (!result)
//                 {
//                     LOG_ERROR("CreatePackage<RspTrade> Failed!");
//                 }
//                 else
//                 {
//                     result->prepare_response(UT_FID_RspTrade, ID_MANAGER->get_id());
//                 }
//             }
//             else
//             {
//                 LOG_ERROR("Cann't Get Src Data " + pReqTrade->symbol_);
//             }
//         }
//         else
//         {
//             string error_msg =  "No Trade Data For " + string(pReqTrade->symbol_) ;
            
//             result = CreatePackage<RspErrorMsg>(error_msg, 1, pReqTrade->socket_id_, pReqTrade->socket_type_);
//             result->prepare_response(UT_FID_RspErrorMsg, ID_MANAGER->get_id());
//             LOG_ERROR(error_msg);
//         }
//         return result;     
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("unkonwn exception!");
//     }    
// }

// std::vector<KlineDataPtr> KlineProcess::get_trade_kline_data(string symbol,  int freq_base, int start_time, int end_time)
// {
//     try
//     {
//          std::vector<KlineDataPtr> result;
//         bool need_more_data = false;
//         std::map<type_tick, KlineDataPtr> cur_src_data;
//         int request_end_time = end_time;

//         if (kline_data_.find(symbol) == kline_data_.end() || 
//             kline_data_[symbol].find(freq_base) ==  kline_data_[symbol].end())
//         {
//             need_more_data = true;      
//         }
//         else if (kline_data_[symbol][freq_base].begin()->first > start_time)
//         {
//             request_end_time = kline_data_[symbol][freq_base].begin()->first;
//             need_more_data = true;
//             cur_src_data = kline_data_[symbol][freq_base];
//         }
//         else
//         {
//             cur_src_data = kline_data_[symbol][freq_base];
//         }

//         if (need_more_data)
//         {
//             vector<KlineData> append_result;
//             HubInterface::get_kline("", symbol.c_str(), freq_base, start_time, request_end_time, append_result);


//             for (KlineData& kline_data:append_result)
//             {
//                 result.emplace_back(boost::make_shared<KlineData>(kline_data));
//             }
//         }
//         else
//         {

//         }

//         if (cur_src_data.size() > 0)
//         {
//             // cout << "\nCached Data: " <<endl;
//             std::map<type_tick, KlineDataPtr>::iterator iter = cur_src_data.begin();
//             while (iter != cur_src_data.end() && iter->first < start_time)
//             {
//                 ++iter;
//             }

//             while(iter != cur_src_data.end() && iter->first <= end_time ) 
//             {
//                 result.push_back(iter->second);     
//                 ++iter;
//             }
//         }
//         return result;    
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("Unknonwn Exception!");
//     }      
// }


// void compute_trade_data(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr);
// void KlineProcess::compute_trade_data(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr)
// {
//     try
//     {
//         update_trade_data(curTradeDataPtr);
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("unkonwn exception!");
//     }        
// }


// map<string, map<ID_TYPE, TradeDataUpdatePtr>>               updated_trade_data_map_;
// void update_new_trade(TradeDataPtr curTradeDataPtr);
// void KlineProcess::update_new_trade(TradeDataPtr curTradeDataPtr)
// {
//     try
//     {
//         string symbol = curTradeDataPtr->symbol_;
//         if (oneday_updated_kline_data_.find(symbol) == oneday_updated_kline_data_.end() || oneday_updated_kline_data_[symbol].is_empty())
//         {
//             curTradeDataPtr->high_ = 0;
//             curTradeDataPtr->low_ = 0;
//         }
//         else
//         {
//             TimeKlineData& cur_time_data = oneday_updated_kline_data_[string(curTradeDataPtr->symbol_)];
//             curTradeDataPtr->high_ = curTradeDataPtr->price_ > cur_time_data.high_ ? curTradeDataPtr->price_: cur_time_data.high_;
//             curTradeDataPtr->low_ = curTradeDataPtr->price_ < cur_time_data.low_ ? curTradeDataPtr->price_ : cur_time_data.low_;

//             type_tick high_time = curTradeDataPtr->high_ == curTradeDataPtr->price_ ? curTradeDataPtr->time_ : cur_time_data.high_time_;
//             type_tick low_time = curTradeDataPtr->low_ == curTradeDataPtr->price_ ? curTradeDataPtr->time_ : cur_time_data.low_time_;

//             curTradeDataPtr->change_ = curTradeDataPtr->price_.get_value() - cur_time_data.start_price_.get_value();
//             curTradeDataPtr->change_rate_ = curTradeDataPtr->change_ / cur_time_data.start_price_.get_value();   
//         }

//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
//     catch(...)
//     {
//         LOG_ERROR("unkonwn exception!");
//     }       
// }


// void check_websocket_trade_req(ReqTradePtr pReqTrade);
// void KlineProcess::check_websocket_trade_req(ReqTradePtr pReqTrade)
// {
//     try
//     {
//         string new_symbol = pReqTrade->symbol_;
        
//         std::lock_guard<std::mutex> lk(trade_wss_con_map_mutex_);

//         if (trade_wss_con_map_.find(pReqTrade->socket_id_) != trade_wss_con_map_.end() 
//         && new_symbol != trade_wss_con_map_[pReqTrade->socket_id_])
//         {
//             string ori_symbol = trade_wss_con_map_[pReqTrade->socket_id_];

//             std::lock_guard<std::mutex> lk(sub_updated_trade_set_mutex_);
//             if (updated_trade_data_map_.find(ori_symbol) != updated_trade_data_map_.end()
//             && updated_trade_data_map_[ori_symbol].find(pReqTrade->socket_id_) != updated_trade_data_map_[ori_symbol].end())
//             {
//                 updated_trade_data_map_[ori_symbol].erase(pReqTrade->socket_id_);

//                 LOG_DEBUG("ReqTrade updated_trade_data_map_  Erase " + ori_symbol + " socket: " + std::to_string(pReqTrade->socket_id_));
//             }
//         }         

//         trade_wss_con_map_[pReqTrade->socket_id_] = new_symbol;

//         TradeDataUpdatePtr pTradeDataUpdate = boost::make_shared<TradeDataUpdate>(*pReqTrade);

//         std::lock_guard<std::mutex> lg(sub_updated_trade_set_mutex_);
//         updated_trade_data_map_[new_symbol][pReqTrade->socket_id_] = pTradeDataUpdate;         

//         // cout << "ReqTrade trade_wss_con_map_ add  " << pReqTrade->symbol_ << " socket: " << pReqTrade->socket_id_ << endl;
//     }
//     catch(const std::exception& e)
//     {
//         stringstream stream_msg;
//         stream_msg << "[E] KlineProcess::check_websocket_trade_req " << e.what() << "\n";
//         LOG_ERROR(stream_msg.str());
//     }
//     catch(...)
//     {
//         std::stringstream stream_obj;
//         stream_obj << "[E] KlineProcess::check_websocket_trade_req: unkonwn exception! " << "\n";
//         LOG_ERROR(stream_obj.str());
//     }       
// }

// void get_append_data(type_tick start_time, type_tick end_time, int data_count, vector<KlineData>& append_result);
// map<string, vector<KlineDataUpdate>>                        updated_kline_data_map_;
// bool delete_kline_request_connect(string symbol, ID_TYPE socket_id);
// void check_websocket_subinfo(ReqKLineDataPtr pReqKlineData);

// if (updated_kline_data_map_.find(symbol) != updated_kline_data_map_.end())
// {
//     for (auto& kline_update:updated_kline_data_map_[symbol])
//     {            
//         int req_fre = kline_update.reqkline_data.frequency_;

//         if (kline_data->frequency_ > req_fre) continue;

//         if (!kline_update.kline_data_)
//         {
//             kline_update.kline_data_ = boost::make_shared<KlineData>(*kline_data);
//         }

//         KlineDataPtr& last_kline = kline_update.kline_data_;

//         if (last_kline->is_clear())
//         {
//             assign(last_kline->symbol, kline_data->symbol);
//             assign(last_kline->exchange, kline_data->exchange);
//             assign(last_kline->px_open, kline_data->px_open);
//             assign(last_kline->px_close, kline_data->px_close);
//             assign(last_kline->px_high, kline_data->px_high);
//             assign(last_kline->px_low, kline_data->px_low);
//             assign(last_kline->volume, kline_data->volume);

//             last_kline->clear_ = false; 
//         }
//         else
//         {
//             last_kline->px_close = kline_data->px_close;
//             last_kline->px_low = last_kline->px_low > kline_data->px_low ? kline_data->px_low: last_kline->px_low;
//             last_kline->px_high = last_kline->px_high < kline_data->px_high ? kline_data->px_high: last_kline->px_high;
//             assign(last_kline->symbol, kline_data->symbol);                
//         }

//         if (kline_data->index - last_kline->index >= last_kline->frequency_)
//         {
//             // 推送更新作为下一个 k 线时间数据;

//             last_kline->index = kline_data->index;
//             KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);

//             std::stringstream s_log;
//             s_log << "Kline Update New : " << get_sec_time_str(cur_kline_data->index) << " " 
//             << "fre: " << cur_kline_data->frequency_ << " "
//             <<"open: " << cur_kline_data->px_open.get_value() << " " 
//             <<"close: " << cur_kline_data->px_close.get_value() << " "
//             <<"high: " << cur_kline_data->px_high.get_value() << " "
//             <<"low: " << cur_kline_data->px_low.get_value() << " \n";
//             LOG_DEBUG(s_log.str());
            
//             PackagePtr rsp_package = GetNewRspKLineDataPackage(kline_update.reqkline_data, cur_kline_data, ID_MANAGER->get_id());

//             if (rsp_package)
//             {
//                 process_engine_->deliver_response(rsp_package);
//             }
//             else
//             {
//                 LOG_ERROR("KlineProcess::update_subed_kline_data GetNewRspKLineDataPackage Failed!");
//             }
//             last_kline->clear();
//         }
//         else
//         {
//             // 推送更新作为当前 k 线时间数据;
//             KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);
//             // cur_kline_data->index = kline_update.last_update_time_;

//             std::stringstream s_log;
//             s_log << "Kline Update old : " << get_sec_time_str(cur_kline_data->index) << " "
//             << "fre: " << cur_kline_data->frequency_ << " "
//             <<"open: " << cur_kline_data->px_open.get_value() << " " 
//             <<"close: " << cur_kline_data->px_close.get_value() << " "
//             <<"high: " << cur_kline_data->px_high.get_value() << " "
//             <<"low: " << cur_kline_data->px_low.get_value() << " \n";
//             LOG_DEBUG(s_log.str());

//             PackagePtr rsp_package = GetNewRspKLineDataPackage(kline_update.reqkline_data, cur_kline_data, ID_MANAGER->get_id());
//             if (rsp_package)
//             {
//                 process_engine_->deliver_response(rsp_package);
//             }
//             else
//             {
//                 LOG_ERROR("KlineProcess::update_subed_kline_data GetNewRspKLineDataPackage Failed!");
//             }

//         }
//     }
// }

// std::map<ID_TYPE, string>                                   wss_con_map_;  
// std::mutex                                                  wss_con_map_mutex_;
// void KlineProcess::check_websocket_subinfo(ReqKLineDataPtr pReqKlineData)
// {
//     try
//     {
//         std::lock_guard<std::mutex> lk(wss_con_map_mutex_);
//         if (wss_con_map_.find(pReqKlineData->socket_id_) != wss_con_map_.end())
//         {
//             // delete_kline_request_connect(wss_con_map_[pReqKlineData->socket_id_], pReqKlineData->socket_id_);
//         }
//         wss_con_map_[pReqKlineData->socket_id_] = pReqKlineData->symbol_;
//     }
//     catch(const std::exception& e)
//     {
//         std::stringstream stream_obj;
//         stream_obj << "[E] KlineProcess::check_websocket_subinfo: " << e.what() << "\n";
//         LOG_ERROR(stream_obj.str());
//     } 
// }


// bool KlineProcess::delete_kline_request_connect(string symbol, ID_TYPE socket_id)
// {
//     try
//     {
//         LOG_DEBUG(string("KlineProcess::delete_kline_request_connect ") + symbol + " " + std::to_string(socket_id));

//         std::lock_guard<std::mutex> lk(sub_updated_kline_map_mutex_);
//         if (updated_kline_data_map_.find(symbol) != updated_kline_data_map_.end())
//         {
//             vector<KlineDataUpdate>& kline_updater = updated_kline_data_map_[symbol];

//             // int pos = 0;
//             // for (; pos < kline_updater.size(); ++pos)
//             // {
//             //     if (kline_updater[pos].reqkline_data.socket_id_ == socket_id) break;
//             // }

//             // if (pos != kline_updater.size())
//             // {
//             //     std::stringstream stream_obj;
//             //     stream_obj << "[Kline Update]: Erase Websocket " 
//             //             << kline_updater[pos].reqkline_data.symbol_ << " " 
//             //             << kline_updater[pos].reqkline_data.frequency_ << " "
//             //             << kline_updater[pos].reqkline_data.socket_id_ << "\n";
//             //     LOG_DEBUG(stream_obj.str());    

//             //     while (pos < kline_updater.size()-1)
//             //     {
//             //         kline_updater[pos] = kline_updater[pos+1];
//             //         ++pos;
                    
//             //     }
//             //     kline_updater.pop_back();
//             // }

//             vector<KlineDataUpdate>::iterator iter = kline_updater.begin();
//             for(;iter != kline_updater.end(); ++iter)
//             {
//                 if ((*iter).reqkline_data.socket_id_ == socket_id) break;
//             }

//             if (iter != kline_updater.end())
//             {
//                 std::stringstream stream_obj;
//                 stream_obj << "[Kline Update]: Erase Websocket " 
//                         << (*iter).reqkline_data.symbol_ << " " 
//                         << (*iter).reqkline_data.frequency_ << " "
//                         << (*iter).reqkline_data.socket_id_ << "\n";
//                 LOG_DEBUG(stream_obj.str());
//                 updated_kline_data_map_[symbol].erase(iter);
//             }
//         }
//         return true;
//     }
//     catch(const std::exception& e)
//     {
//         std::stringstream stream_obj;
//         stream_obj << "[E] KlineProcess::delete_kline_request_connect: " << e.what() << "\n";
//         LOG_ERROR(stream_obj.str());
//     }    
//     catch (...)
//     {
//         std::stringstream stream_obj;
//         stream_obj << "[E] KlineProcess::delete_kline_request_connect: Unkonw Exceptions " << "\n";
//         LOG_ERROR(stream_obj.str());        
//     }
// }

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
//         std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
//     }
    
// }