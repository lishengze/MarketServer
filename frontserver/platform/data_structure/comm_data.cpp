#include "comm_data.h"
#include "../front_server_declare.h"
#include "../log/log.h"
#include "../util/tools.h"

string RspSymbolListData::get_json_str()
{
    nlohmann::json json_data;
    nlohmann::json symbol_json;

    int i = 0;

    string specified_first_symbol = "BTC_USDT";

    if (symbols_.find(specified_first_symbol) != symbols_.end())
    {
        symbols_.erase(specified_first_symbol);
        symbol_json[i++] = specified_first_symbol;
    }
    
    for (string symbol:symbols_)
    {
        if (symbol.length()==0 ||symbol == "") continue;
        
        symbol_json[i++] = symbol;
    }
    json_data["symbol"] = symbol_json;    

    json_data["type"] = SYMBOL_LIST;

    return json_data.dump();
}


RspRiskCtrledDepthData& RspRiskCtrledDepthData::operator=(const RspRiskCtrledDepthData& other)
{
    socket_id_ = other.socket_id_;
    socket_type_ = other.socket_type_;
    http_response_ = other.http_response_;
    websocket_ = other.websocket_;    

    depth_data_ = other.depth_data_;
    for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
        bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
    }

    return *this;
}

void RspRiskCtrledDepthData::set(const SDepthData& depth_data, ID_TYPE socket_id, COMM_TYPE socket_type)
{
    socket_id_ = socket_id;
    socket_type_ = socket_type;

    // cout << "RspRiskCtrledDepthData::init SDepthData" << endl;

    // std::lock_guard<std::mutex> lg(mutex_);

    memcpy(&depth_data_, &depth_data, sizeof(SDepthData));

    // depth_data_ = *depth_data;

    for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume + ask_accumulated_volume_[i-1];
    }

    for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume + bid_accumulated_volume_[i-1];
    }

}

void RspRiskCtrledDepthData::set_depth(const SDepthData& depth_data)
{
    try
    {
        memcpy(&depth_data_, &depth_data, sizeof(SDepthData));

        for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
        {
            ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume + ask_accumulated_volume_[i-1];
        }

        for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
        {
            bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume + bid_accumulated_volume_[i-1];
        }

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string RspRiskCtrledDepthData::get_json_str()
{
    string result;
    nlohmann::json json_data;
    json_data["symbol"] = string(depth_data_.symbol);
    json_data["exchange"] = string(depth_data_.exchange);
    json_data["tick"] = depth_data_.tick;
    json_data["seqno"] = depth_data_.seqno;
    json_data["ask_length"] = depth_data_.ask_length;
    json_data["bid_length"] = depth_data_.bid_length;   

    nlohmann::json asks_json;
    for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = append_zero(depth_data_.asks[i].price.get_str_value(), depth_data_.precise);
        depth_level_atom[1] = append_zero(depth_data_.asks[i].volume.get_str_value(), depth_data_.vprecise);
        depth_level_atom[2] = append_zero(ask_accumulated_volume_[i].get_str_value(), depth_data_.vprecise);
        asks_json[i] = depth_level_atom;
    }
    json_data["asks"] = asks_json;

    nlohmann::json bids_json;
    for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = append_zero(depth_data_.bids[i].price.get_str_value(), depth_data_.precise);
        depth_level_atom[1] = append_zero(depth_data_.bids[i].volume.get_str_value(), depth_data_.vprecise);
        depth_level_atom[2] = append_zero(bid_accumulated_volume_[i].get_str_value(), depth_data_.vprecise);
        bids_json[i] = depth_level_atom;
    }
    json_data["bids"] = bids_json;
    json_data["type"] = MARKET_DATA_UPDATE;

    result = json_data.dump(); 
    
    return result;
}

string RspKLineData::get_json_str()
{
    try
    {
        string result;
        nlohmann::json json_data;       
        if (is_update_)
        {
            json_data["type"] = KLINE_UPDATE;
        } 
        else
        {
            json_data["type"] = KLINE_RSP;
        }
        
        json_data["symbol"] = string(symbol_);
        json_data["start_time"] = start_time_;
        json_data["end_time"] = end_time_;
        json_data["frequency"] = frequency_;
        json_data["data_count"] = data_count_;

        int i = 0;
        nlohmann::json detail_data;
        for (KlineDataPtr atom_data:kline_data_vec_)
        {
            nlohmann::json tmp_json;
            tmp_json["open"] = atom_data->px_open.get_value();
            tmp_json["high"] = atom_data->px_high.get_value();
            tmp_json["low"] = atom_data->px_low.get_value();
            tmp_json["close"] = atom_data->px_close.get_value();
            tmp_json["volume"] = atom_data->volume.get_value();
            tmp_json["tick"] = atom_data->index;
            detail_data[i++] = tmp_json;
        }
        json_data["data"] = detail_data;
        return json_data.dump();
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] RspKLineData::get_json_str: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    return nullptr;
}

string RspEnquiry::get_json_str()
{
    string result;

    nlohmann::json json_data;
    nlohmann::json data_list;
    nlohmann::json data;

    data["price"] = std::to_string(price_);
    data["symbol"] = string(symbol_);
    data["type"] = string(RSP_ENQUIRY);

    json_data["code"] = 0;
    json_data["msg"] = "";
    json_data["data"] = data;    

    return json_data.dump(); 
}

string RspErrorMsg::get_json_str()
{
    nlohmann::json json_data;

    json_data["code"] = err_id_;
    json_data["msg"] = err_msg_;   
    json_data["type"] = RSP_ERROR;   

    return json_data.dump();
}

string RspTrade::get_json_str()
{
    nlohmann::json json_data;

    json_data["type"] = TRADE;
    json_data["symbol"] = string(symbol_);
    json_data["price"] = price_.get_str_value();
    json_data["volume"] = volume_.get_str_value(); 
    json_data["change"] = simplize_string(std::to_string(change_));
    json_data["change_rate"] = simplize_string(set_double_string_scale(std::to_string(change_rate_), PERSENT_DOT_NUMB)); 
    json_data["high"] = high_.get_str_value();
    json_data["low"] = low_.get_str_value();

    return json_data.dump();    
}
