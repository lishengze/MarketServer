#include "tool.h"

void print_quote(const SInnerQuote& quote)
{
    std::stringstream s_s;
    s_s << "\n" << quote.exchange << "." << quote.symbol <<" ask.size: " << quote.asks.size() << ", bid.size: " << quote.bids.size() << "\n";
    // int test_numb = 5;    
    // s_s << "------------- asks info: first " << test_numb << " data \n";
    // int i = 0;

    // for (auto iter = quote.asks.begin();iter != quote.asks.end() && i < test_numb; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }
    // s_s << "+++++++++ last" << test_numb <<  " data +++++++++" << endl;
    // i = 0;
    // for (auto iter = quote.asks.rbegin();iter != quote.asks.rend() && i < test_numb; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }    

    // s_s << "***************** bids info: first " << test_numb << " data \n";
    // i = 0;
    // for (auto iter = quote.bids.rbegin();iter != quote.bids.rend() && i < test_numb; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }
    // s_s << "+++++++++last" << test_numb <<  " data+++++++++" << endl;
    // i = 0;
    // for (auto iter = quote.bids.begin();iter != quote.bids.end() && i < test_numb; ++iter, ++i)
    // {
    //     s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    // }  

    s_s << "------------- asks info \n";
    for (auto iter = quote.asks.begin();iter != quote.asks.end(); ++iter)
    {
        s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    }

    s_s << "************* bids info \n";
    for (auto iter = quote.bids.rbegin();iter != quote.bids.rend(); ++iter)
    {
        s_s << iter->first.get_value() << ": " << iter->second.total_volume.get_value() << endl;
    }    

    LOG_DEBUG(s_s.str());
}
