#include "data_struct.h"


EnhancedDepthData::EnhancedDepthData(const SDepthData* depth_data):type_{"market_data"}
{
    init(depth_data);
}

void EnhancedDepthData::init(const SDepthData* depth_data)
{
    // cout << "EnhancedDepthData::init 0" << endl;
    std::lock_guard<std::mutex> lg(mutex_);

    memcpy(&depth_data_, depth_data, sizeof(SDepthData));

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

void EnhancedDepthData::set_json_str()
{
    nlohmann::json json_data;
    json_data["symbol"] = depth_data_.symbol;
    json_data["exchange"] = depth_data_.exchange;
    json_data["tick"] = depth_data_.tick;
    json_data["seqno"] = depth_data_.seqno;
    json_data["ask_length"] = depth_data_.ask_length;
    json_data["bid_length"] = depth_data_.bid_length;   

    nlohmann::json asks_json;
    for (int i = 0; i < depth_data_.ask_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth_data_.asks[i].price.get_value();
        depth_level_atom[1] = depth_data_.asks[i].volume.get_value();
        depth_level_atom[2] = ask_accumulated_volume_[i];
        asks_json[i] = depth_level_atom;
    }
    json_data["asks"] = asks_json;

    nlohmann::json bids_json;
    for (int i = 0; i < depth_data_.bid_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth_data_.bids[i].price.get_value();
        depth_level_atom[1] = depth_data_.bids[i].volume.get_value();
        depth_level_atom[2] = bid_accumulated_volume_[i];
        bids_json[i] = depth_level_atom;
    }
    json_data["bids"] = bids_json;

    json_str_ = json_data.dump(); 
}

void SymbolData::set_json_str()
{
    try
    {
        nlohmann::json json_data;
        nlohmann::json symbol_json;

        int i = 0;
        for (string symbol:symbols_)
        {
            cout << "symbol: " << symbol << endl;
            symbol_json[i++] = symbol;
        }
        json_data["symbols"] = symbol_json;    
        json_str_ = json_data.dump();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(...)
    {
        std::cerr << "SymbolData::set_json_str Unknow Error" << endl;
    }
    

}

string SymbolData::get_json_str()
{
    // set_json_str();
    return json_str_;
}