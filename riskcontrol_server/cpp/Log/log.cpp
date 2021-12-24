#include "log.h"
#include "pandora/util/time_util.h"
#include "../updater_quote.h"

RiskLog::RiskLog()
{
    
}

RiskLog::~RiskLog()
{

}

void RiskLog::record_input_info(const string& info, const SEData& quote)
{
    try
    {
        record_input_info(info);
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }    
}
