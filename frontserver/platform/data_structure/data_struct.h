#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"

#include "hub_struct.h"


enum class COMM_TYPE {
    HTTP = 0,
    HTTPS,
    WEBSOCKET,
    WEBSECKETS,
};

class HttpRequest
{
    public:


    HttpRequestAtom*    http_request_{nullptr};
    std::mutex          mutex_;
};

class HttpResponse
{
    public:


    HttpResponseAtom*    http_response_{nullptr};
    std::mutex          mutex_;
};

class WebsocketClassThreadSafe
{
    public:

    WebsocketClassThreadSafe(WebsocketClass* ws)
    {
        ws_ = ws;
    }

    void send(string msg)
    {
        std::lock_guard<std::mutex> lk(mutex);
        ws_->send(msg, uWS::OpCode::TEXT);
    }

    void end()
    {
        std::lock_guard<std::mutex> lk(mutex);
        ws_->end();
    }

    WebsocketClass*    ws_{nullptr};
    std::mutex             mutex_;
};

struct Socket
{
    Socket(HttpResponse* res, HttpRequest* req):comm_type{COMM_TYPE::HTTP}, http_response_{res}, http_request_{req}
    {}

    Socket(WebsocketClassThreadSafe* ws):comm_type{COMM_TYPE::WEBSOCKET}, websocket_{ws}{}

    Socket(HttpResponse* res=nullptr, HttpRequest* req=nullptr, WebsocketClassThreadSafe* ws=nullptr):
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

    COMM_TYPE           comm_type = COMM_TYPE::HTTP;
    HttpResponse*       http_response_{nullptr};
    HttpRequest*        http_request_{nullptr};
    WebsocketClassThreadSafe*     websocket_{nullptr};
};



const long UT_FID_EnhancedDepthData = 0x10002;
class EnhancedDepthData:public boost::enable_shared_from_this<EnhancedDepthData>
{
    public:
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


        double ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        double bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_EnhancedDepthData;  

    private:
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

const long UT_FID_ReqDepthData = 0x10004;
class ReqDepthData:public Socket
{

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

    symbol_type symbol;
    double open_;
    double high_;
    double low_;
    double close_;
    double volume_;    
    type_tick tick_;
};
FORWARD_DECLARE_PTR(AtomKlineData);

const long UT_FID_ReqKLineData = 0x10005;
class ReqKLineData:public Socket
{
    public:
    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int freq, 
                HttpResponse* res=nullptr, WebsocketClassThreadSafe* ws=nullptr):
    Socket(res, nullptr, ws)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(frequency_, freq);
    }

    void reset(const ReqKLineData& other)
    {
        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(frequency_, other.frequency_);

        http_response_ = other.http_response_;
        websocket_ = other.websocket_;
        comm_type = other.comm_type;
    }

    void set (string symbol, type_tick start_time, type_tick end_time, int freq, 
            HttpResponse* res=nullptr, WebsocketClassThreadSafe* ws=nullptr)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(frequency_, freq);

        http_response_ = res;
        websocket_ = ws;

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

    public: 
        symbol_type         symbol_;
        type_tick           start_time_;
        type_tick           end_time_;
        type_tick           append_end_time_;

        frequency_type      frequency_;

        static const long Fid = UT_FID_ReqKLineData;
};
FORWARD_DECLARE_PTR(ReqKLineData);

const long UT_FID_RspKLineData = 0x10006;
class RspKLineData:public Socket
{
    public: 
        symbol_type                         symbol_;
        type_tick                           start_time_;
        type_tick                           end_time_;
        frequency_type                      frequency_;
        std::vector<AtomKlineDataPtr>       kline_data_vec_;

        static const long Fid = UT_FID_RspKLineData;
};
FORWARD_DECLARE_PTR(RspKLineData);