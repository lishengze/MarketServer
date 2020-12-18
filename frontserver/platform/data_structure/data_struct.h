#pragma once
#include "../front_server_declare.h"
#include "hub_struct.h"
#include <mutex>

#include <boost/shared_ptr.hpp>

using symbol_type = std::string;
using frequency_type = unsigned int;

enum class COMM_TYPE {
    HTTP = 0,
    HTTPS,
    WEBSOCKET,
    WEBSECKETS,
};


struct Socket
{
    Socket(HttpResponse* res, HttpRequest* req):comm_type{COMM_TYPE::HTTP}, http_response_{res}, http_request_{req}
    {}

    Socket(WebsocketClass* ws):comm_type{COMM_TYPE::WEBSOCKET}, websocket_{ws}{}

    Socket(HttpResponse* res=nullptr, HttpRequest* req=nullptr, WebsocketClass* ws=nullptr):
    http_response_{res}, http_request_{req}, websocket_{ws}
    {
        if (websocket_)
        {
            cout << "IS COMM_TYPE::WEBSOCKET" << endl;
            comm_type = COMM_TYPE::WEBSOCKET;
        }

        if (http_response_)
        {
            cout << "IS COMM_TYPE::HTTP" << endl;
            comm_type = COMM_TYPE::HTTP;
        }
    }

    COMM_TYPE comm_type = COMM_TYPE::HTTP;
    HttpResponse*       http_response_{nullptr};
    HttpRequest*        http_request_{nullptr};
    WebsocketClass*     websocket_{nullptr};
};

const long UT_FID_EnhancedDepthData = 0x10002;
class EnhancedDepthData:public boost::enable_shared_from_this<EnhancedDepthData>
{
    public:
        EnhancedDepthData():type_{"market_data"} {}
        EnhancedDepthData(const SDepthData* depth_data);

        EnhancedDepthData(const EnhancedDepthData& other);

        EnhancedDepthData & operator=(const EnhancedDepthData& other);

        void init(const SDepthData* depth_data);

        virtual ~EnhancedDepthData() {}

        boost::shared_ptr<EnhancedDepthData> get_object() 
        { 
            boost::shared_ptr<EnhancedDepthData> shared_this(this); 
            return shared_this;
        }

        void set_json_str();
        string get_json_str() {
            set_json_str();
            return json_str_;
        }

        double ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        double bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_EnhancedDepthData;  

        string                    type_{"market_data"};

    private:
        string                    json_str_;
        std::mutex                mutex_;
        

};
FORWARD_DECLARE_PTR(EnhancedDepthData);

const long UT_FID_SymbolData = 0x10003;
class SymbolData
{
    public:
        SymbolData():type_{"symbol_update"} {}
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

        void set_type(string type_str) { type_ = type_str;}

        static const long Fid = UT_FID_SymbolData; 

        void set_json_str();
        string get_json_str();

        string                    type_;

    private:
        std::mutex                mutex_;
        std::set<std::string>     symbols_;
        string                    json_str_;
        
};

struct AtomKlineData
{
    AtomKlineData(double open, double high, double low, double close, double volume):
        open_{open}, close_{close}, high_{high}, low_{low}, volume_{volume} {}
        
    AtomKlineData(KlineData& kline_data)
    {
        // cout << "Reference AtomKlineData " << endl;
        open_ = kline_data.px_open.get_value();
        high_ = kline_data.px_high.get_value();
        low_ = kline_data.px_low.get_value();
        close_ = kline_data.px_close.get_value();
        volume_ = kline_data.volume.get_value();
        tick_ = kline_data.index;
    }

    AtomKlineData() {
        // cout << "default AtomKlineData " << endl;
    }

    string symbol;
    double open_;
    double high_;
    double low_;
    double close_;
    double volume_;    
    type_tick tick_;
};
FORWARD_DECLARE_PTR(AtomKlineData);

const long UT_FID_ReqKLineData = 0x10004;
class ReqKLineData:public Socket
{
    public:
    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int freq, 
                HttpResponse* res=nullptr, WebsocketClass* ws=nullptr):
    Socket(res, nullptr, ws)
    {
        symbol_ = symbol;
        start_time_ = start_time;
        end_time_ = end_time;
        frequency_ = freq;
    }

    void reset(const ReqKLineData& other)
    {
        symbol_ = other.symbol_;
        start_time_ = other.start_time_;
        end_time_ = other.end_time_;
        frequency_ = other.frequency_;

        http_response_ = other.http_response_;
        websocket_ = other.websocket_;
        comm_type = other.comm_type;
    }

    public: 
        string              symbol_;
        type_tick           start_time_;
        type_tick           end_time_;
        type_tick           append_end_time_;

        int                 frequency_;

        static const long Fid = UT_FID_ReqKLineData;
};

const long UT_FID_RspKLineData = 0x10005;
class RspKLineData:public Socket
{
    public: 
        string                              symbol_;
        type_tick                           start_time_;
        type_tick                           end_time_;
        int                                 frequency_;
        std::vector<AtomKlineDataPtr>       kline_data_vec_;

        static const long Fid = UT_FID_RspKLineData;
};