#include "tool.h"
#include "../Log/log.h"
#include "pandora/util/float_util.h"
#include "pandora/util/time_util.h"

bool filter_zero_volume(SDepthQuote& quote)
{
    try
    {
        bool result = false;
        std::list<map<SDecimal, SDepth>::iterator> delete_iter_list;
        for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
        {
            if (utrade::pandora::equal(iter->second.volume.get_value(), 0))
            {
                delete_iter_list.push_back(iter);
            }
        }

        for (auto& iter:delete_iter_list)
        {
            quote.asks.erase(iter);
        }

        delete_iter_list.clear();
        for (auto iter = quote.bids.begin();iter != quote.bids.end(); ++iter)
        {
            if (utrade::pandora::equal(iter->second.volume.get_value(), 0))
            {
                delete_iter_list.push_back(iter);
            }    
        }
        for (auto& iter:delete_iter_list)
        {
            quote.bids.erase(iter);
        }

        if (quote.asks.size()==0 && quote.bids.size()==0)
        {
            result = true;
        }
        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void print_quote(const SDepthQuote& quote)
{
    try
    {
       std::stringstream s_s;
       s_s << "\n" << quote.exchange << "." << quote.symbol << ", ask: " << quote.asks.size() << ", bid: " << quote.bids.size() << "\n";

       
        if (quote.asks.size() > 0)
        {
             s_s << "------------- asks info \n";
            for(auto& iter:quote.asks)
            {
                s_s << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";

                    // for (auto iter2:iter.second.volume_by_exchanges)
                    // {
                    //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                    // }

            }
        }

        if (quote.bids.size() > 0)
        {
            s_s << "------------- bid_info: \n";
            for(auto& iter:quote.bids)
            {
                s_s << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";

                    // for (auto iter2:iter.second.volume_by_exchanges)
                    // {
                    //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                    // }           
            }      
        }

       LOG_DEBUG(s_s.str()); 
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

string quote_str(const SDepthQuote& quote)
{
    try
    {
       std::stringstream s_s;
       s_s << "\n" << quote.exchange << "." << quote.symbol << ", ask: " << quote.asks.size() << ", bid: " << quote.bids.size() << "\n";

       
        if (quote.asks.size() > 0)
        {
             s_s << "------------- asks info \n";
            for(auto& iter:quote.asks)
            {
                s_s << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";

                    // for (auto iter2:iter.second.volume_by_exchanges)
                    // {
                    //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                    // }

            }
        }

        if (quote.bids.size() > 0)
        {
            s_s << "------------- bid_info: \n";
            for(auto& iter:quote.bids)
            {
                s_s << iter.first.get_value() << ": " << iter.second.volume.get_value() << "\n";

                    // for (auto iter2:iter.second.volume_by_exchanges)
                    // {
                    //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                    // }           
            }      
        }

       return s_s.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }       
}

string quote_str(const SDepthQuote& quote, int count)
{
    try
    {
        std::stringstream s_s;
        s_s << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";
                
        if (count > 0)
        {
            int i = 0;

            if (quote.asks.size() > 0)
            {
                s_s << "------------- asks info: first " << count << " data \n";
                
                for (auto iter = quote.asks.begin();iter != quote.asks.end() && i < count; ++iter, ++i)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }
                s_s << "+++++++++ last" << count <<  " data +++++++++" << endl;
                i = 0;
                for (auto iter = quote.asks.rbegin();iter != quote.asks.rend() && i < count; ++iter, ++i)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }                    
            }

            if (quote.bids.size() > 0)
            {
                s_s << "***************** bids info: first " << count << " data \n";
                i = 0;
                for (auto iter = quote.bids.rbegin();iter != quote.bids.rend() && i < count; ++iter, ++i)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }
                s_s << "+++++++++last" << count <<  " data+++++++++" << endl;
                i = 0;
                for (auto iter = quote.bids.begin();iter != quote.bids.end() && i < count; ++iter, ++i)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }    
            }          
        }
        else
        {
            if (quote.asks.size() > 0)
            {
                s_s << "------------- asks info \n";
                for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }
            }

            if (quote.bids.size() > 0)
            {
                s_s << "************* bids info \n";
                for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.volume.get_value() << endl;
                }    
            }
        }

        return s_s.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

string get_sec_time_str(unsigned long time)
{
    return utrade::pandora::ToSecondStr(time * NANOSECONDS_PER_SECOND, "%Y-%m-%d %H:%M:%S");
}

struct SInnerDepth {
    SDecimal total_volume; // 总挂单量，用于下发行情
    map<TExchange, SDecimal> exchanges;
    //double amount_cost; // 余额消耗量

    SInnerDepth() {
    }

    void mix_exchanges(const SInnerDepth& src, double bias, uint32 kind=1) 
    {
        if (kind == 1 && bias > -100)
        {
            for( const auto& v : src.exchanges ) 
            {                
                exchanges[v.first] += (v.second * (1 + bias)) > 0 ? (v.second * (1 + bias)) : 0;
            }
        }
        else if (kind == 2)
        {
            for( const auto& v : src.exchanges ) 
            {                
                exchanges[v.first] += (v.second + bias) > 0 ? (v.second + bias) :0;
            }
        }

        total_volume = 0;
        for( const auto& v : exchanges ) {
            total_volume += v.second;
        }
    }

    void set_total_volume()
    {
        total_volume = 0;
        for( const auto& v : exchanges ) {
            total_volume += v.second;
        }        
    }
};

struct SInnerQuote {
    string exchange;
    string symbol;
    type_tick time_origin;      // 交易所原始时间
    type_tick time_arrive_at_streamengine;   // se收到的时间
    type_tick time_produced_by_streamengine;    // se处理完发送的时间
    type_tick time_arrive;  // rc收到的时间
    type_seqno seq_no;
    uint32 precise;
    uint32 vprecise;
    map<SDecimal, SInnerDepth> asks;
    map<SDecimal, SInnerDepth> bids;

    SInnerQuote() {
        seq_no = 0;
        precise = 0;
        vprecise = 0;
        time_origin = time_arrive_at_streamengine = time_produced_by_streamengine = time_arrive = 0;
    }

    void get_asks(vector<pair<SDecimal, SInnerDepth>>& depths) const {
        depths.clear();
        for( auto iter = asks.begin() ; iter != asks.end() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }

    void get_bids(vector<pair<SDecimal, SInnerDepth>>& depths) const {
        depths.clear();
        for( auto iter = bids.rbegin() ; iter != bids.rend() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }
};

void quotedata_to_innerquote(const SEData& src, SInnerQuote& dst) {
    dst.exchange = src.exchange();
    dst.symbol = src.symbol();
    dst.precise = src.price_precise();
    dst.vprecise = src.volume_precise();
    dst.time_origin = src.time();
    dst.time_arrive_at_streamengine = src.time_arrive();
    dst.time_produced_by_streamengine = src.time_produced_by_streamengine();
    dst.time_arrive = get_miliseconds();
    
    //vassign(dst.seq_no, src.msg_seq());
    // 卖盘
    for( int i = 0 ; i < src.asks_size() ; ++i ) {
        const SEDepth& src_depth = src.asks(i);
        SDecimal price = SDecimal::parse_by_raw(src_depth.price().base(), src_depth.price().prec());
        SInnerDepth depth;
        for( auto v : src_depth.data() ) {
            depth.exchanges[v.first] = SDecimal::parse_by_raw(v.second.base(), v.second.prec());
        }
        depth.total_volume = SDecimal::parse_by_raw(src_depth.volume().base(), src_depth.volume().prec());
        dst.asks[price] = depth;

        // dst.asks[price].set_total_volume();
    }
    // 买盘
    for( int i = 0 ; i < src.bids_size() ; ++i ) {
        const SEDepth& src_depth = src.bids(i);
        SDecimal price = SDecimal::parse_by_raw(src_depth.price().base(), src_depth.price().prec());
        SInnerDepth depth;
        for( auto v : src_depth.data() ) {
            depth.exchanges[v.first] = SDecimal::parse_by_raw(v.second.base(), v.second.prec());
        }
        depth.total_volume = SDecimal::parse_by_raw(src_depth.volume().base(), src_depth.volume().prec());
        dst.bids[price] = depth;

        // dst.asks[price].set_total_volume();
    }
}

void print_inner_quote(const SInnerQuote& quote)
{
    std::stringstream s_s;
    s_s << "\n" << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";

    if (quote.asks.size() > 0)
    {
        s_s << "------------- asks info \n";
        for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
        {
            s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << " \n" ;

            // for (auto iter2:iter->second.exchanges)
            // {
            //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
            // }
            
        }
    }

    if (quote.bids.size() > 0)
    {
        s_s << "************* bids info \n";
        for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
        {
            s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << " \n" ;

            // for (auto iter2:iter->second.exchanges)
            // {
            //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
            // }        
        }    
    }    

    LOG_DEBUG(s_s.str());
}

string inner_quote_str(const SInnerQuote& quote)
{
    try
    {
        std::stringstream s_s;
        s_s << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";

        if (quote.asks.size() > 0)
        {
            s_s << "------------- asks info \n";
            for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << " \n" ;

                // for (auto iter2:iter->second.exchanges)
                // {
                //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                // }
                
            }
        }

        if (quote.bids.size() > 0)
        {
            s_s << "************* bids info \n";
            for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << " \n" ;

                // for (auto iter2:iter->second.exchanges)
                // {
                //     s_s << iter2.first << " " << iter2.second.get_str_value() << " \n";
                // }        
            }    
        }    

        return s_s.str();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    

}

void print_sedata(const SEData& sedata)
{
    try
    {
        SInnerQuote inner_quote;

        quotedata_to_innerquote(sedata, inner_quote);

        print_inner_quote(inner_quote);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string sedata_str(const SEData& sedata)
{
    try
    {
        SInnerQuote inner_quote;

        quotedata_to_innerquote(sedata, inner_quote);

        

        return inner_quote_str(inner_quote);
    }
    catch(const std::exception& e)
    {
       LOG_ERROR(e.what());
    }    
}

bool filter_zero_volume(SEData& sedata)
{
    try
    {
        SInnerQuote quote;
        quotedata_to_innerquote(sedata, quote);

        bool result = false;
        std::list<map<SDecimal, SInnerDepth>::iterator> delete_iter_list;
        for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
        {
            if (utrade::pandora::equal(iter->second.total_volume.get_value(), 0))
            {
                delete_iter_list.push_back(iter);
            }
        }

        for (auto& iter:delete_iter_list)
        {
            quote.asks.erase(iter);
        }

        delete_iter_list.clear();
        for (auto iter = quote.bids.begin();iter != quote.bids.end(); ++iter)
        {
            if (utrade::pandora::equal(iter->second.total_volume.get_value(), 0))
            {
                delete_iter_list.push_back(iter);
            }    
        }
        for (auto& iter:delete_iter_list)
        {
            quote.bids.erase(iter);
        }

        if (quote.asks.size()==0 && quote.bids.size()==0)
        {
            result = true;
        }
        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

string klines_str(vector<KlineData>& kline_list)
{
    try
    {
        std::stringstream stream_obj;

        for(auto& kline:kline_list)
        {
            stream_obj << kline_str(kline);    
        }

        return stream_obj.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

string kline_str(KlineData& kline)
{
    try
    {
        std::stringstream stream_obj;

        stream_obj  << "[K-Kine] SRC " << get_sec_time_str(kline.index)  
                    << ", " << kline.exchange << ", "<< kline.symbol << ","
                    << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
                    << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value() << "\n"; 

        return stream_obj.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void filter_kline_data(vector<KlineData>& kline_list)
{
    try
    {
        if (kline_list.size() == 0) return;

        std::list<vector<KlineData>::iterator> error_kline_list;

        string symbol = kline_list[0].symbol;
        string exchange = kline_list[0].exchange;


        vector<KlineData> valid_kline_list;
        for(vector<KlineData>::iterator iter = kline_list.begin(); iter != kline_list.end(); ++iter)
        {
            if (is_kline_valid(*iter))
            {
                error_kline_list.push_back(iter);
            }
            else
            {
                valid_kline_list.push_back(std::move(*iter));
            }
        }        

        if (error_kline_list.size() > 0)
        {
            LOG_WARN(symbol+ "." + exchange + " erase " + std::to_string(error_kline_list.size()) + " invalid data");
        }

        kline_list.swap(valid_kline_list);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool is_kline_valid(KlineData& kline)
{
    bool result = false;
    try
    {
        if (kline.px_open<=0 || kline.px_close<=0 || kline.px_high<=0 || kline.px_low<=0 )
        {
            result = true;
        }
        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return false;
}