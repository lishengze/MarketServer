#include "tools.h"
#include "../front_server_declare.h"
#include "pandora/util/json.hpp"
#include "../log/log.h"
#include "pandora/util/time_util.h"
#include "id.hpp"
#include "../ErrorDefine.hpp"

void copy_sdepthdata(SDepthData* des, const SDepthData* src)
{
    // *des = *src;
    memcpy(des, src, sizeof (SDepthData));
}

void copy_klinedata(KlineData* des, const KlineData* src)
{
    memcpy(des, src, sizeof (KlineData));
}

void copy_enhanced_data(RspRiskCtrledDepthData* des, const RspRiskCtrledDepthData* src)
{
    memcpy(des, src, sizeof(RspRiskCtrledDepthData));
}

string SDepthDataToJsonStr(const SDepthData& depth)
{
    nlohmann::json json_data;
    json_data["symbol"] = string(depth.symbol);
    json_data["exchange"] = string(depth.exchange);
    json_data["tick"] = depth.tick;
    json_data["seqno"] = depth.seqno;
    json_data["ask_length"] = depth.ask_length;
    json_data["bid_length"] = depth.bid_length;   

    nlohmann::json asks;
    for (int i = 0; i < depth.ask_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.asks[i].price.get_value();
        depth_level_atom[1] = depth.asks[i].volume.get_value();
        asks[i] = depth_level_atom;
    }
    json_data["asks"] = asks;

    nlohmann::json bids;
    for (int i = 0; i < depth.bid_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.bids[i].price.get_value();
        depth_level_atom[1] = depth.bids[i].volume.get_value();
        bids[i] = depth_level_atom;
    }
    json_data["bids"] = bids;

    return json_data.dump(); 
}

string SymbolsToJsonStr(std::set<std::string> symbols, string type)
{
    nlohmann::json json_data;
    nlohmann::json symbol_json;

    int i = 0;

    string specified_first_symbol = "BTC_USDT";

    if (symbols.find(specified_first_symbol) != symbols.end())
    {
        symbols.erase(specified_first_symbol);
        symbol_json[i++] = specified_first_symbol;
    }
    
    for (string symbol:symbols)
    {
        if (symbol.length()==0 ||symbol == "") continue;
        
        symbol_json[i++] = symbol;
    }
    json_data["symbol"] = symbol_json;    

    json_data["type"] = type;

    return json_data.dump();
}

string SymbolsToJsonStr(RspSymbolListData& symbol_data, string type)
{
    std::set<std::string>& symbols = symbol_data.get_symbols();

    return SymbolsToJsonStr(symbols, type);
}

string RspRiskCtrledDepthDataToJsonStr(RspRiskCtrledDepthData& en_data, string type)
{
    string result;
    nlohmann::json json_data;
    json_data["symbol"] = string(en_data.depth_data_.symbol);
    json_data["exchange"] = string(en_data.depth_data_.exchange);
    json_data["tick"] = en_data.depth_data_.tick;
    json_data["seqno"] = en_data.depth_data_.seqno;
    json_data["ask_length"] = en_data.depth_data_.ask_length;
    json_data["bid_length"] = en_data.depth_data_.bid_length;   

    nlohmann::json asks_json;
    for (int i = 0; i < en_data.depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = en_data.depth_data_.asks[i].price.get_value();
        depth_level_atom[1] = en_data.depth_data_.asks[i].volume.get_value();
        depth_level_atom[2] = en_data.ask_accumulated_volume_[i].get_value();
        asks_json[i] = depth_level_atom;
    }
    json_data["asks"] = asks_json;

    nlohmann::json bids_json;
    for (int i = 0; i < en_data.depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = en_data.depth_data_.bids[i].price.get_value();
        depth_level_atom[1] = en_data.depth_data_.bids[i].volume.get_value();
        depth_level_atom[2] = en_data.bid_accumulated_volume_[i].get_value();
        bids_json[i] = depth_level_atom;
    }
    json_data["bids"] = bids_json;
    json_data["type"] = type;

    result = json_data.dump(); 
    
    return result;
}

void append_kline_to_klinePtr(std::vector<KlineDataPtr>& des, std::vector<KlineData>& src)
{
    for (KlineData atom: src)
    {
        KlineDataPtr atom_ptr = boost::make_shared<KlineData>(atom);
        des.emplace_back(atom_ptr);
    }
}

string get_error_send_rsp_string(string err_msg)
{
    string result = "";
    try
    {
        nlohmann::json json_obj;
        json_obj["type"] = "error";
        json_obj["error_id"] = LOST_TYPE_ITEM;
        json_obj["error_msg"] = err_msg;
        return json_obj.dump();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unkonwn exception!");
    }

    return result;
}

string get_heartbeat_str()
{
    string result = "";

    try
    {
        nlohmann::json json_obj;
        json_obj["type"] = "heartbeat";
        json_obj["time"] = utrade::pandora::NanoTimeStr();
        return json_obj.dump();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("unkonwn exception!");
    }
        
    return result;
}

vector<string>  split(const string& src,const string& delim) 
{
    vector<string> des;  
    if("" == src) return  des;  
      
    string tmpstrs = src + delim;
    size_t pos;  
    size_t size = tmpstrs.size();  
  
    for (size_t i = 0; i < size; ++i) {  
        pos = tmpstrs.find(delim, i); 
        if( pos < size) {
            des.push_back(tmpstrs.substr(i, pos - i)); 
            i = pos + delim.size() - 1;  
        }  
    }  
    return des;   
}  

string set_double_string_scale(string ori_data, int num)
{
    string result = ori_data;
    vector<string> vec_str = split(ori_data, ".");
    if (vec_str.size() == 2)
    {
        string dot_numb_str = vec_str[1];
        if (num > 0)
        {
            if (dot_numb_str.length() > num)
            {                        
                dot_numb_str = dot_numb_str.substr(0, num);
                // if (num == 4)
                // {
                //     cout << vec_str[0] << "  " << vec_str[1] << " " << vec_str[1].length() << " " << dot_numb_str << endl;
                // }
                
        
            }        
            result = vec_str[0] + "." + dot_numb_str;
            // if (num == 4) cout << vec_str[0] << " " << dot_numb_str << " " << result << endl;
        }
    }
    // if (num == 4) cout << result << endl;
    return result;
}

string simplize_string(string ori_data)
{
    string result = ori_data;
    if (result.find(".")!=std::string::npos)
    {
        int end_pos = result.length() - 1;
        for (; end_pos > 0 && result[end_pos] != '.'; --end_pos)
        {
            if (result[end_pos] != '0') 
            {
                // cout << "result: " << result << endl;
                break;
            }            
        }
        result = result.substr(0, end_pos+1);
    }
    return result;
}

string append_zero(string ori_data, int count)
{
    string result = ori_data;
    vector<string> vec_str = split(ori_data, ".");
    if (vec_str.size() == 2)
    {
        string dot_numb_str = vec_str[1];
        if (dot_numb_str.length() > count)
        {
            dot_numb_str = dot_numb_str.substr(0, count);
        }
        else if (dot_numb_str.length() < count)
        {
            while(dot_numb_str.length() != count)
            {
                dot_numb_str += "0";
            }
        }
        result = vec_str[0] + "." + dot_numb_str;
    }
    return result;    
}

string get_package_str(unsigned int package_tid)
{
    try
    {
        string result;
        switch (package_tid)
        {
        case UT_FID_ReqSymbolListData:
            result = "ReqSymbolListData";
            break;

        case UT_FID_ReqRiskCtrledDepthData:
            result = "ReqRiskCtrledDepthData";
            break;

        case UT_FID_ReqKLineData:
            result = "ReqKLineData";
            break;

        case UT_FID_ReqEnquiry:
            result = "ReqEnquiry";
            break;

        case UT_FID_ReqTrade:
            result = "ReqTrade";
            break;

        case UT_FID_RspSymbolListData:
            result = "RspSymbolListData";
            break;

        case UT_FID_RspRiskCtrledDepthData:
            result = "RspRiskCtrledDepthData";
            break;

        case UT_FID_RspKLineData:
            result = "RspKLineData";
            break;

        case UT_FID_RspTrade:
            result = "RspTrade";
            break;

        case UT_FID_RspEnquiry:
            result = "RspEnquiry";
            break;

        case UT_FID_RspErrorMsg:
            result = "RspErrorMsg";
            break;

        case UT_FID_SDepthData:
            result = "SourceDepthData";
            break;        

        case UT_FID_KlineData:
            result = "SourceKlineData";
            break;      

        case UT_FID_TradeData:
            result = "SourceTradeData";
            break;                                                                                                                                                          
        
        default:
            break;
        }

        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
    return "";
}