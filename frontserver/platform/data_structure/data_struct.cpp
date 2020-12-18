#include "data_struct.h"


EnhancedDepthData::EnhancedDepthData(const SDepthData* depth_data)
{
    init(depth_data);
}

EnhancedDepthData::EnhancedDepthData(const EnhancedDepthData& other)
{
    cout << "EnhancedDepthData::EnhancedDepthData " << endl;

    // memcpy(&depth_data_, &other.depth_data_, sizeof(SDepthData));
    // memcpy(ask_accumulated_volume_, other.ask_accumulated_volume_, sizeof(double) * DEPCH_LEVEL_COUNT);
    // memcpy(bid_accumulated_volume_, other.bid_accumulated_volume_, sizeof(double) * DEPCH_LEVEL_COUNT);

    depth_data_ = other.depth_data_;
    for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
        bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
    }
}

EnhancedDepthData & EnhancedDepthData::operator=(const EnhancedDepthData& other)
{
    cout << "EnhancedDepthData::Operator = " << endl;

    // memcpy(&depth_data_, &other.depth_data_, sizeof(SDepthData));
    // memcpy(ask_accumulated_volume_, other.ask_accumulated_volume_, sizeof(double) * DEPCH_LEVEL_COUNT);
    // memcpy(bid_accumulated_volume_, other.bid_accumulated_volume_, sizeof(double) * DEPCH_LEVEL_COUNT);

    depth_data_ = other.depth_data_;
    for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
        bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
    }
}

void EnhancedDepthData::init(const SDepthData* depth_data)
{
    // cout << "EnhancedDepthData::init SDepthData" << endl;

    std::lock_guard<std::mutex> lg(mutex_);

    memcpy(&depth_data_, depth_data, sizeof(SDepthData));

    // depth_data_ = *depth_data;

    // cout << "EnhancedDepthData::init 1" << endl;

    for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume.get_value() + ask_accumulated_volume_[i-1];
    }

    // cout << "EnhancedDepthData::init 2" << endl;

    for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume.get_value() + bid_accumulated_volume_[i-1];
    }

    // cout << "EnhancedDepthData::init 3" << endl;
}

string SymbolData::get_json_str()
{
    // set_json_str();
    return json_str_;
}