#include "comm_data.h"
#include "../front_server_declare.h"


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
        ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume.get_value() + ask_accumulated_volume_[i-1];
    }

    // cout << "RspRiskCtrledDepthData::init 2" << endl;

    for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume.get_value() + bid_accumulated_volume_[i-1];
    }

    // cout << "RspRiskCtrledDepthData::init 3" << endl;
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
