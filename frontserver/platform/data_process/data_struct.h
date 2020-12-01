#pragma once
#include "../front_server_declare.h"
#include "hub_struct.h"
#include <mutex>

#include <boost/shared_ptr.hpp>

const long UT_FID_EnhancedDepthData = 0x10002;
class EnhancedDepthData:public boost::enable_shared_from_this<EnhancedDepthData>
{
    public:
        EnhancedDepthData(const SDepthData* depth_data);

        void init(const SDepthData* depth_data);

        virtual ~EnhancedDepthData() {}

        boost::shared_ptr<EnhancedDepthData> get_object() 
        { 
            boost::shared_ptr<EnhancedDepthData> shared_this(this); 
            return shared_this;
        }

        void set_json_str();
        string get_json_str();

        double ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        double bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_EnhancedDepthData;  

    private:
        string  json_str_;

};

const long UT_FID_SymbolData = 0x10003;
class SymbolData
{
    public:
        void add_symbol(string symbol)
        {
            symbols_.emplace(symbol);
        }

        void set_symbols(std::set<std::string>& symbols)
        {
            std::lock_guard<std::mutex> lg(mutex_);

            symbols_ = symbols;
        }

        std::set<std::string>& get_symbols() { return symbols_;}

        static const long Fid = UT_FID_SymbolData; 

        void set_json_str();
        string get_json_str();


    private:
        std::mutex                mutex_;
        std::set<std::string>     symbols_;
        string                    json_str_;
};