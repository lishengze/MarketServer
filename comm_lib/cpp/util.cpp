#include "util.h"

COMM_NAMESPACE_START

string get_kline_topic(string exchange, string symbol)
{
    try
    {
        return string(KLINE_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return "";
}


string get_depth_topic(string exchange, string symbol)
{
    try
    {
        return string(DEPTH_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_trade_topic(string exchange, string symbol)
{
    try
    {
        return string(TRADE_TYPE) + TYPE_SEPARATOR + symbol+ SYMBOL_EXCHANGE_SEPARATOR  + exchange;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

void set_depth_json(nlohmann::json& ask_json, const map<SDecimal, SDepth>& depth)
{
    try
    {
        for (auto iter:depth)
        {
            if (iter.second.volume_by_exchanges.empty())
            {
                ask_json[iter.first.get_str_value()] = iter.second.volume.get_value();
            }
            else
            {
                nlohmann::json depth_json;
                nlohmann::json volume_by_exchanges_json;
                depth_json["volume"] = iter.second.volume.get_value();
                for (auto iter2:iter.second.volume_by_exchanges)
                {
                    volume_by_exchanges_json[iter2.first] = iter2.second.get_value();
                }

                depth_json["volume_by_exchanges"] = volume_by_exchanges_json;

                ask_json[iter.first.get_str_value()] = depth_json;
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }        
}


string get_depth_jsonstr(const SDepthQuote& depth)
{
    try
    {
        nlohmann::json json_data;            

        json_data["Symbol"] = depth.symbol;
        json_data["Exchange"] = depth.exchange;
        json_data["Type"] = "snap";
        json_data["TimeArrive"] = std::to_string(depth.origin_time);
        json_data["Msg_seq_symbol"] = depth.sequence_no;

        nlohmann::json ask_json;
        nlohmann::json bid_json;

        set_depth_json(ask_json, depth.asks);
        set_depth_json(bid_json, depth.bids);

        json_data["AskDepth"] = ask_json;
        json_data["BidDepth"] = bid_json;

        return json_data.dump();        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_kline_jsonstr(const KlineData& kline)
{
    try
    {
        nlohmann::json json_data;            

        json_data[0] = kline.index;
        json_data[1] = kline.px_open.get_value();
        json_data[2] = kline.px_high.get_value();
        json_data[3] = kline.px_low.get_value();
        json_data[4] = kline.px_close.get_value();
        json_data[5] = kline.volume.get_value();
        
        json_data[6] = kline.symbol;
        json_data[7] = kline.exchange;
        json_data[8] = kline.resolution;

        return json_data.dump();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

string get_trade_jsonstr(const TradeData& trade)
{
    try
    {
        nlohmann::json json_data;            

        json_data["Time"] = trade.time;
        json_data["LastPx"] = trade.price.get_value();
        json_data["Qty"] = trade.volume.get_value();
        json_data["Exchange"] = trade.exchange;
        json_data["Symbol"] = trade.symbol;

        return json_data.dump();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
    return "";
}

COMM_NAMESPACE_END