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

class HttpRequestThreadSafe
{
    public:

    HttpRequest*    http_request_{nullptr};
    std::mutex      mutex_;
};
FORWARD_DECLARE_PTR(HttpRequestThreadSafe);

class HttpResponseThreadSafe
{
    public:

    HttpResponseThreadSafe(HttpResponse* res)
    {
        http_response_ = res;
    }

    void end(const string& info)
    {
        std::lock_guard<std::mutex> lk(mutex);
        http_response_->end(info);
    }

    HttpResponse*    http_response_{nullptr};
    std::mutex       mutex_;
};

FORWARD_DECLARE_PTR(HttpResponseThreadSafe);

struct WSData
{
    bool              is_alive{false};
    std::set<string>  sub_symbol_set;
};

class WebsocketClassThreadSafe
{
    public:

    WebsocketClassThreadSafe(WebsocketClass* ws)
    {
        ws_ = ws;
    }

    void send(const string& msg)
    {
        std::lock_guard<std::mutex> lk(mutex);
        cout << "ws send: " << msg << endl;
        ws_->send(msg, uWS::OpCode::TEXT);
    }

    void end()
    {
        std::lock_guard<std::mutex> lk(mutex);
        ws_->end();
    }

    bool operator < (const WebsocketClassThreadSafe& other)
    {
        return ws_ < other.ws_;
    }

    bool operator == (const WebsocketClassThreadSafe& other)
    {
        return ws_ == other.ws_;
    }    

    bool is_alive() {
        std::lock_guard<std::mutex> lk(mutex_);
        return user_data_.is_alive;
    }

    void set_alive(bool value)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        user_data_.is_alive = value;
    }

    void clear_sub_symbol_list()
    {
        std::lock_guard<std::mutex> lk(mutex_);
        user_data_.sub_symbol_set.clear();
    }

    void add_sub_symbol(std::string symbol)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        user_data_.sub_symbol_set.emplace(symbol);
    }

    bool is_symbol_subed(std::string symbol)
    {
        std::lock_guard<std::mutex> lk(mutex_);

        if (user_data_.sub_symbol_set.find(symbol) != user_data_.sub_symbol_set.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    WebsocketClass* get_ws() { return ws_;}


    private:
        WSData                 user_data_;
        WebsocketClass*        ws_{nullptr};
        std::mutex             mutex_;
};

FORWARD_DECLARE_PTR(WebsocketClassThreadSafe);

class LessWebsocketClassThreadSafePtr
{
public:
    bool operator() (const WebsocketClassThreadSafePtr& a, const WebsocketClassThreadSafePtr& b) const
    {
        return a->get_ws() < b->get_ws();
    }
};

struct Socket
{
    Socket(HttpResponse* res):comm_type{COMM_TYPE::HTTP}
    {
        http_response_ = boost::make_shared<HttpResponseThreadSafe>(res);
    }

    Socket(HttpResponseThreadSafePtr res):comm_type{COMM_TYPE::HTTP}
    {
        http_response_ = res;
    }    

    Socket(WebsocketClass* ws):comm_type{COMM_TYPE::WEBSOCKET}
    {
        websocket_ = boost::make_shared<WebsocketClassThreadSafe>(ws);
    }

    Socket(WebsocketClassThreadSafePtr ws):comm_type{COMM_TYPE::WEBSOCKET}
    {
        websocket_ = ws;
    }    

    Socket(HttpResponse* res=nullptr, WebsocketClass* ws=nullptr)
    {
        if (ws)
        {
            cout << "IS COMM_TYPE::WEBSOCKET" << endl;
            websocket_ = boost::make_shared<WebsocketClassThreadSafe>(ws);
            comm_type = COMM_TYPE::WEBSOCKET;
        }

        if (res)
        {
            cout << "IS COMM_TYPE::HTTP" << endl;
            http_response_ = boost::make_shared<HttpResponseThreadSafe>(res);
            comm_type = COMM_TYPE::HTTP;
        }
    }

    Socket(HttpResponseThreadSafePtr res=nullptr, WebsocketClassThreadSafePtr ws=nullptr)
    {
        if (ws)
        {
            cout << "IS COMM_TYPE::WEBSOCKET" << endl;
            websocket_ = ws;
            comm_type = COMM_TYPE::WEBSOCKET;
        }

        if (res)
        {
            cout << "IS COMM_TYPE::HTTP" << endl;
            http_response_ = res;
            comm_type = COMM_TYPE::HTTP;
        }
    }    

    COMM_TYPE                       comm_type = COMM_TYPE::HTTP;
    HttpResponseThreadSafePtr       http_response_{nullptr};
    WebsocketClassThreadSafePtr     websocket_{nullptr};
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
    private:
        std::mutex                mutex_;
        std::set<std::string>     symbols_;
};

const long UT_FID_ReqDepthData = 0x10004;
class ReqDepthData:public Socket
{
    public:
    ReqDepthData(string symbol, HttpResponse* res=nullptr, WebsocketClass* ws=nullptr):
    Socket(res, ws)
    {
        assign(symbol_, symbol);
    }

    ReqDepthData(const ReqDepthData& other):Socket(other.http_response_, other.websocket_)
    {
        assign(symbol_, other.symbol_);
    }

    symbol_type symbol_;

    static const long Fid = UT_FID_ReqDepthData;
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
    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
                HttpResponse* res=nullptr, WebsocketClass* ws=nullptr):
    Socket(res, ws)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
    }

    void reset(const ReqKLineData& other)
    {
        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(frequency_, other.frequency_);
        assign(data_count_, other.data_count_);

        http_response_ = other.http_response_;
        websocket_ = other.websocket_;
        comm_type = other.comm_type;
    }
  
    public: 
        symbol_type         symbol_;
        type_tick           start_time_{0};
        type_tick           end_time_{0};
        type_tick           append_end_time_{0};
        int                 data_count_{-1};

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
        int                                 data_count_;
        std::vector<AtomKlineDataPtr>       kline_data_vec_;

        static const long Fid = UT_FID_RspKLineData;
};
FORWARD_DECLARE_PTR(RspKLineData);