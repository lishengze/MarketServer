#include "basic.h"

#include "base_data_stuct.h"

#include "pandora/util/path_util.h"

inline bool is_kline_valid(const KlineData& kline) 
{
    try
    {
        return !(kline.index < 1000000000 || kline.index > 1900000000 ||
                 kline.px_open<=0 || kline.px_close<=0 || kline.px_high<=0 || kline.px_low<=0);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return false;
}

inline bool is_trade_valid(const TradeData& trade)
{
    try
    {
        return trade.time_ > 1000000000000000000 && trade.time_ < 1900000000000000000;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return false;    
}
