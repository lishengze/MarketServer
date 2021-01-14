#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"
#include "../util/id.hpp"
#include "base_data.h"
#include "hub_struct.h"



const long UT_FID_RspSymbolListData = 0x10002;
class RspSymbolListData
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

        static const long Fid = UT_FID_RspSymbolListData; 
    private:
        std::mutex                mutex_;
        std::set<std::string>     symbols_;
};

const long UT_FID_ReqRiskCtrledDepthData = 0x10003;
class ReqRiskCtrledDepthData:public Socket
{
    public:
    ReqRiskCtrledDepthData(string symbol, HttpResponse* res=nullptr, WebsocketClass* ws=nullptr):
    Socket(res, ws)
    {
        assign(symbol_, symbol);
    }

    ReqRiskCtrledDepthData(const ReqRiskCtrledDepthData& other):Socket(other.http_response_, other.websocket_)
    {
        assign(symbol_, other.symbol_);
    }

    symbol_type symbol_;

    static const long Fid = UT_FID_ReqRiskCtrledDepthData;
};

const long UT_FID_RspRiskCtrledDepthData = 0x10004;
class RspRiskCtrledDepthData:public boost::enable_shared_from_this<RspRiskCtrledDepthData>
{
    public:
        RspRiskCtrledDepthData(const SDepthData* depth_data);

        RspRiskCtrledDepthData(const RspRiskCtrledDepthData& other);

        RspRiskCtrledDepthData & operator=(const RspRiskCtrledDepthData& other);

        void init(const SDepthData* depth_data);

        virtual ~RspRiskCtrledDepthData() {}

        boost::shared_ptr<RspRiskCtrledDepthData> get_object() 
        { 
            boost::shared_ptr<RspRiskCtrledDepthData> shared_this(this); 
            return shared_this;
        }


        double ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        double bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_RspRiskCtrledDepthData;  

    private:
        std::mutex                mutex_;
};
FORWARD_DECLARE_PTR(RspRiskCtrledDepthData);

const long UT_FID_ReqEnquiry = 0x10005;
class ReqEnquiry:public Socket
{
    public:
        ReqEnquiry(string symbol, double volumn, double amount, int type,
                    HttpResponseThreadSafePtr res=nullptr, WebsocketClassThreadSafePtr ws=nullptr):
        Socket(res, ws)
        {
            assign(symbol_, symbol);
            assign(volume_, volumn);
            assign(amount_, amount);
        }

        void reset(const ReqEnquiry& other)
        {
            assign(symbol_, other.symbol_);
            assign(volume_, other.volume_);
            assign(amount_, other.amount_);         

            http_response_ = other.http_response_;
            websocket_     = other.websocket_;
            comm_type      = other.comm_type;               
        }

        void set(string symbol, double volumn, double amount, int type,
                 HttpResponseThreadSafePtr res=nullptr, WebsocketClassThreadSafePtr ws=nullptr)
        {
            assign(symbol_, symbol);
            assign(volume_, volumn);
            assign(amount_, amount); 
            assign(type_, type);

            if (res != nullptr)
            {
                http_response_ = res;
                comm_type = COMM_TYPE::HTTP;
            }
            else if (ws != nullptr)
            {
                websocket_ = ws;
                comm_type = COMM_TYPE::WEBSOCKET;
            }         
        }        
    static const long Fid = UT_FID_ReqEnquiry;

        symbol_type symbol_;
        int         type_{-1};
        double      volume_{-1};
        double      amount_{-1};

    
};
FORWARD_DECLARE_PTR(ReqEnquiry);

const long UT_FID_RspEnquiry = 0x10006;
class RspEnquiry:public Socket
{
    public:
        RspEnquiry(string symbol, double price, HttpResponseThreadSafePtr res=nullptr, WebsocketClassThreadSafePtr ws=nullptr):
        Socket(res, ws)
        {
            assign(price_, price);
            assign(symbol_, symbol);
        }

        void set(string symbol, double price, HttpResponseThreadSafePtr res=nullptr, 
                 WebsocketClassThreadSafePtr ws=nullptr)
        {
            assign(price_, price);
            assign(symbol_, symbol);    

            if (res != nullptr)
            {
                http_response_ = res;
                comm_type = COMM_TYPE::HTTP;
            }
            else if (ws != nullptr)
            {
                websocket_ = ws;
                comm_type = COMM_TYPE::WEBSOCKET;
            }                         
        }

        string get_json_str();
        
    static const long Fid = UT_FID_RspEnquiry;
    
    private:
        double          price_;
        symbol_type     symbol_;  
};
FORWARD_DECLARE_PTR(RspEnquiry);

const long UT_FID_ReqKLineData = 0x10007;
class ReqKLineData:public Socket
{
    public:
    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, ID_TYPE ws_id,
                HttpResponse* res=nullptr, WebsocketClass* ws=nullptr):
    Socket(res, ws)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(ws_id_, ws_id);
    }

    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, ID_TYPE ws_id,
                 WebsocketClassThreadSafePtr ws):
    Socket(ws)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(ws_id_, ws_id);
    }    

    void set(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
             WebsocketClassThreadSafePtr ws)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);

        websocket_ = ws;
        comm_type = COMM_TYPE::WEBSOCKET;    

        cout << "ReqKLineData::set ws" << ws->get_ws() <<endl;       
    }

    void reset(const ReqKLineData& other)
    {
        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(frequency_, other.frequency_);
        assign(data_count_, other.data_count_);
        assign(ws_id_, other.ws_id_);

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

        ID_TYPE             ws_id_{-1};

        frequency_type      frequency_;

        static const long Fid = UT_FID_ReqKLineData;
};
FORWARD_DECLARE_PTR(ReqKLineData);

const long UT_FID_RspKLineData = 0x10008;
class RspKLineData:public Socket
{
    public: 
        symbol_type                         symbol_;
        type_tick                           start_time_;
        type_tick                           end_time_;
        frequency_type                      frequency_;
        ID_TYPE                             ws_id_{-1};
        int                                 data_count_;
        std::vector<AtomKlineDataPtr>       kline_data_vec_;

        static const long Fid = UT_FID_RspKLineData;
};
FORWARD_DECLARE_PTR(RspKLineData);

const long UT_FID_RspErrorMsg = 0x10009;
class RspErrorMsg:public Socket
{
    public:
        RspErrorMsg(string err_msg, int err_id, 
                    HttpResponseThreadSafePtr res=nullptr, 
                    WebsocketClassThreadSafePtr ws=nullptr):
        Socket(res, ws)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);
        }

        void set(string err_msg, int err_id,  
                 HttpResponseThreadSafePtr res=nullptr, 
                 WebsocketClassThreadSafePtr ws=nullptr)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);

            if (res != nullptr)
            {
                http_response_ = res;
                comm_type = COMM_TYPE::HTTP;
            }
            else if (ws != nullptr)
            {
                websocket_ = ws;
                comm_type = COMM_TYPE::WEBSOCKET;
            }                         
        }

        string get_json_str();

    static const long Fid = UT_FID_RspErrorMsg;
    
    private:
        int             err_id_;
        error_msg_type  err_msg_;
};
FORWARD_DECLARE_PTR(RspErrorMsg);