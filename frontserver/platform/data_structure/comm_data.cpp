#include "comm_data.h"
#include "../front_server_declare.h"
#include "../log/log.h"


RspRiskCtrledDepthData::RspRiskCtrledDepthData(const SDepthData* depth_data)
{
    init(depth_data);
}

RspRiskCtrledDepthData::RspRiskCtrledDepthData(const RspRiskCtrledDepthData& other)
{
    cout << "RspRiskCtrledDepthData::RspRiskCtrledDepthData " << endl;
    
    depth_data_ = other.depth_data_;
    for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
        bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
    }
}

RspRiskCtrledDepthData & RspRiskCtrledDepthData::operator=(const RspRiskCtrledDepthData& other)
{
    cout << "RspRiskCtrledDepthData::Operator = " << endl;

    depth_data_ = other.depth_data_;
    for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
        bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
    }
}

void RspRiskCtrledDepthData::init(const SDepthData* depth_data)
{
    // cout << "RspRiskCtrledDepthData::init SDepthData" << endl;

    std::lock_guard<std::mutex> lg(mutex_);

    memcpy(&depth_data_, depth_data, sizeof(SDepthData));

    // depth_data_ = *depth_data;

    // cout << "RspRiskCtrledDepthData::init 1" << endl;

    for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume + ask_accumulated_volume_[i-1];
    }

    // cout << "RspRiskCtrledDepthData::init 2" << endl;

    for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume + bid_accumulated_volume_[i-1];
    }

    // cout << "RspRiskCtrledDepthData::init 3" << endl;
}

string RspKLineData::get_json_str()
{
    try
    {
        string result;
        nlohmann::json json_data;       
        if (is_update)
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
