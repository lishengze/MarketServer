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
    frequency_list_ = CONFIG->get_frequency_list();
    frequency_numb_ = CONFIG->get_frequency_numb();
    frequency_base_ = CONFIG->get_frequency_base();
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
        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);
        if (pReqKlineData)
        {
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
}

void KlineProcess::response_src_kline_package(PackagePtr package)
{
    try
    {
        if (test_kline_data_) return;

        KlineData* pkline_data = GET_NON_CONST_FIELD(package, KlineData);

        update_kline_data(pkline_data);

        if (pkline_data)
        {
            string cur_symbol = string(pkline_data->symbol);

            for (int cur_frequency: frequency_list_)
            {
                store_kline_data(cur_frequency, pkline_data);
            }

            // if (kline_data_[cur_symbol][frequency_list_[0]].size() > 60) return;            

            // LOG_DEBUG(string("kline_data push ") + cur_symbol + ", "  
            //             + std::to_string(frequency_list_[0]) + ":" + std::to_string(kline_data_[cur_symbol][frequency_list_[0]].size()) + ", "
            //             + get_sec_time_str(pkline_data->index) + "\n");

            // for (auto fre:frequency_list_)
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

void KlineProcess::store_kline_data(int frequency, KlineData* pkline_data)
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

                return;
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

                if (kline_data_[cur_symbol][frequency].size() == frequency_numb_)
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
            }
        }

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

        ReqKLineData * pReqKlineData = GET_NON_CONST_FIELD(package, ReqKLineData);

        stringstream s_obj;
        s_obj << "symbol_: " << pReqKlineData->symbol_ << ", \n"
             << "frequency_: " << pReqKlineData->frequency_ << ", \n"
             << "request start_time: " << pReqKlineData->start_time_ << ", \n"
             << "request end_time: " << pReqKlineData->end_time_ << ", \n"
             << "request data_count: " << pReqKlineData->data_count_ << ", \n";
        LOG_DEBUG(s_obj.str());

        if (kline_data_.find(pReqKlineData->symbol_) != kline_data_.end())
        {
            std::lock_guard<std::mutex> lk(kline_data_mutex_);

            std::map<type_tick, KlineDataPtr> symbol_kline_data = kline_data_[pReqKlineData->symbol_][frequency_base_];
            vector<KlineData> append_result;
            vector<KlineDataPtr> src_kline_data;
            bool is_need_aggregation{true};
            
            if (kline_data_[pReqKlineData->symbol_].find(pReqKlineData->frequency_) != kline_data_[pReqKlineData->symbol_].end())
            {
                cout << "Has frequency_: " << pReqKlineData->frequency_ << endl;
                symbol_kline_data = kline_data_[pReqKlineData->symbol_][pReqKlineData->frequency_];
                is_need_aggregation = false;
            }

            s_obj.clear();
            s_obj << "\n" << pReqKlineData->symbol_ << ", \n"
                 << "kline_data start_time: " << get_sec_time_str(symbol_kline_data.begin()->first) << ", \n"
                 << "kline_data end_time: " << get_sec_time_str(symbol_kline_data.rbegin()->first) << ", \n"
                 << "kline_data data_size: " << symbol_kline_data.size() << "\n";
            // LOG_DEBUG(s_obj.str());

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
                    
                    HubInterface::get_kline("", pReqKlineData->symbol_, 1, pReqKlineData->start_time_, symbol_kline_data.begin()->first, append_result);
                }

                append_kline_to_klinePtr(src_kline_data, append_result);
                get_src_kline_data(src_kline_data, symbol_kline_data, pReqKlineData->start_time_, pReqKlineData->end_time_);
            }
            else
            {
                // cout << "src_kline_data.size: " << src_kline_data.size() << endl;
                int data_count = pReqKlineData->data_count_;
                if (is_need_aggregation)
                {
                    data_count *= (pReqKlineData->frequency_ / frequency_base_);
                }
                cout << "src data_cout: " << data_count << endl;
                get_src_kline_data(src_kline_data, symbol_kline_data, data_count);
            }            

            cout << "src_kline_data.size: " << src_kline_data.size() << endl;

            vector<KlineDataPtr> target_kline_data = compute_target_kline_data(src_kline_data, pReqKlineData->frequency_);

            PackagePtr rsp_package;

            if (target_kline_data.size() > 0)
            {
                rsp_package = GetNewRspKLineDataPackage(pReqKlineData, target_kline_data, ID_MANAGER->get_id());

                rsp_package->prepare_response(UT_FID_RspKLineData, rsp_package->PackageID());                
            }
            else
            {
                stringstream s_obj;
                s_obj << "No KLine Data For " << pReqKlineData->symbol_ 
                      << " fre: " << pReqKlineData->frequency_ 
                      << " data_count: " << pReqKlineData->data_count_;

                string err_msg = s_obj.str();
                int err_id = 1;
                rsp_package = GetRspErrMsgPackage(err_msg, err_id, pReqKlineData->http_response_, pReqKlineData->websocket_);
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

void KlineProcess::get_src_kline_data(vector<KlineDataPtr>& src_kline_data, std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time)
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

void KlineProcess::get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, int data_count)
{
    // result.resize(data_count);
    std::map<type_tick, KlineDataPtr>::iterator iter = symbol_kline_data.begin();

    while (data_count < symbol_kline_data.size())
    {
        iter++;
        data_count++;
    }

    while (iter != symbol_kline_data.end())
    {
        result.emplace_back(iter->second);       
        ++iter;
    }
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
        
        if (atom->index - (*result.rbegin())->index >= frequency)
        {            
            KlineDataPtr cur_data = boost::make_shared<KlineData>(*atom);
            cur_data->px_low = low;
            cur_data->px_high = high;
            cur_data->px_open = open;

            is_first = true;
            result.push_back(cur_data); 

            low = MAX_DOUBLE;
            high = MIN_DOUBLE;
        }
    }
    return result;
}

void KlineProcess::init_test_kline_data()
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

void KlineProcess::init_update_kline_data(PackagePtr rsp_package, ReqKLineData * pReqKlineData)
{
    const RspKLineData* p_rsp_kline_data = GET_FIELD(rsp_package, RspKLineData);

    if (p_rsp_kline_data)
    {
        auto iter = p_rsp_kline_data->kline_data_vec_.rbegin();

        KlineDataUpdatePtr kline_update = boost::make_shared<KlineDataUpdate>(*pReqKlineData);

        updated_kline_data_[string(pReqKlineData->symbol_)].emplace_back(kline_update);          

        kline_update->last_update_time_ = (*iter)->index;         

        cout << pReqKlineData->symbol_ << " Last Update Time: " << get_sec_time_str(kline_update->last_update_time_) << endl; 
    }
}

void KlineProcess::update_kline_data(const KlineData* kline_data)
{
    string symbol = kline_data->symbol;

    if (updated_kline_data_.find(symbol) != updated_kline_data_.end())
    {
        cout << "KlineProcess::update_kline_data " << symbol << endl;

        vector<KlineDataUpdatePtr>& vec = updated_kline_data_[symbol];

        // cout << "vec.size: " << vec.size() << endl;

        for (auto kline_update:vec)
        {                        
            if (!kline_update->kline_data_ )
            {
                kline_update->kline_data_ = boost::make_shared<KlineData>(*kline_data);
            }

            KlineDataPtr last_kline = kline_update->kline_data_;

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

            if (kline_data->index - kline_update->last_update_time_ >= kline_update->req_kline_data_.frequency_)
            {
                cout << "Kline Update Time: " << get_sec_time_str(kline_data->index) << endl;
                KlineDataPtr cur_kline_data = boost::make_shared<KlineData>(*last_kline);
                
                PackagePtr rsp_package = GetNewRspKLineDataPackage(&kline_update->req_kline_data_, cur_kline_data, ID_MANAGER->get_id());

                if (rsp_package)
                {
                    rsp_package->prepare_response(UT_FID_RspKLineData, rsp_package->PackageID());

                    process_engine_->deliver_response(rsp_package);
                }

                kline_update->last_update_time_ = kline_data->index;

                last_kline->clear();
            }
        }
    }
}