#include "tool.h"
#include "pandora/util/float_util.h"

#include "../Log/log.h"

void print_quote(const SInnerQuote& quote)
{
    std::stringstream s_s;
    s_s << "\n" << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";
    // int count = 5;    
    // s_s << "------------- asks info: first " << count << " data \n";
    // int i = 0;

    // for (auto iter = quote.asks.begin();iter != quote.asks.end() && i < count; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }
    // s_s << "+++++++++ last" << count <<  " data +++++++++" << endl;
    // i = 0;
    // for (auto iter = quote.asks.rbegin();iter != quote.asks.rend() && i < count; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }    

    // s_s << "***************** bids info: first " << count << " data \n";
    // i = 0;
    // for (auto iter = quote.bids.rbegin();iter != quote.bids.rend() && i < count; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }
    // s_s << "+++++++++last" << count <<  " data+++++++++" << endl;
    // i = 0;
    // for (auto iter = quote.bids.begin();iter != quote.bids.end() && i < count; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }  

    if (quote.asks.size() > 0)
    {
        s_s << "------------- asks info \n";
        for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
        {
            s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
        }
    }

    if (quote.bids.size() > 0)
    {
        s_s << "************* bids info \n";
        for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
        {
            s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
        }    
    }




    LOG_DEBUG(s_s.str());
}

string quote_str(const SInnerQuote& quote, int count)
{
    try
    {
        std::stringstream s_s;
        s_s << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";
                
        if (count > 0)
        {
            s_s << "------------- asks info: first " << count << " data \n";
            int i = 0;

            for (auto iter = quote.asks.begin();iter != quote.asks.end() && i < count; ++iter, ++i)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
            }
            s_s << "+++++++++ last" << count <<  " data +++++++++" << endl;
            i = 0;
            for (auto iter = quote.asks.rbegin();iter != quote.asks.rend() && i < count; ++iter, ++i)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
            }    

            s_s << "***************** bids info: first " << count << " data \n";
            i = 0;
            for (auto iter = quote.bids.rbegin();iter != quote.bids.rend() && i < count; ++iter, ++i)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
            }
            s_s << "+++++++++last" << count <<  " data+++++++++" << endl;
            i = 0;
            for (auto iter = quote.bids.begin();iter != quote.bids.end() && i < count; ++iter, ++i)
            {
                s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
            }              
        }
        else
        {
            if (quote.asks.size() > 0)
            {
                s_s << "------------- asks info \n";
                for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
                }
            }

            if (quote.bids.size() > 0)
            {
                s_s << "************* bids info \n";
                for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
                {
                    s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
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

bool filter_zero_volume(SInnerQuote& quote, std::mutex& mutex_)
{
    try
    {
        std::lock_guard<std::mutex> lk(mutex_);

        return filter_zero_volume(quote);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool filter_zero_volume(SInnerQuote& quote)
{
    try
    {
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
        std::cerr << e.what() << '\n';
    }    
}

string get_work_dir_name(string path)
{
    try
    {
        string result = "";

        string::size_type i = path.find_last_of("/") + 1;

        if (i != string::npos)
        {
            result = path.substr(0, i);
        }

        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

string get_program_name(string path)
{
    try
    {
        string result = path;

        string::size_type i = path.find_last_of("/") + 1;

        if (i != string::npos)
        {
            result = path.substr(i, path.size()-i);
        }

        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}