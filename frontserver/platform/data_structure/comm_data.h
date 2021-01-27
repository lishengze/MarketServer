#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"
#include "../util/id.hpp"
#include "base_data.h"
#include "hub_struct.h"

const long UT_FID_ReqSymbolListData = 0x10002;
class ReqSymbolListData:public Socket
{
    public:
        ReqSymbolListData(ID_TYPE socket_id, COMM_TYPE socket_type): Socket(socket_id, socket_type)
        {

        }

        ReqSymbolListData(const ReqSymbolListData& other): Socket(other.socket_id_, other.socket_type_)
        {
            assign(is_canacel_request_, other.is_canacel_request_);
        }

        void set(ID_TYPE socket_id, COMM_TYPE socket_type, bool is_canacel_request=false)
        {
            socket_id_ = socket_id;
            socket_type_ = socket_type;    
            is_canacel_request_ =  is_canacel_request;
        }

        bool                is_canacel_request_{false};

        static const long Fid = UT_FID_ReqSymbolListData; 
};
FORWARD_DECLARE_PTR(ReqSymbolListData);

const long UT_FID_RspSymbolListData = 0x10003;
class RspSymbolListData:public Socket
{
    public:
        void add_symbol(string symbol)
        {
            symbols_.emplace(symbol);
        }

        void set(std::set<std::string>& symbols, ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            assign(symbols_, symbols);

            socket_id_ = socket_id;
            socket_type_ = socket_type;
        }

        std::set<std::string>& get_symbols() { return symbols_;}

        string get_json_str();

        static const long Fid = UT_FID_RspSymbolListData; 
    private:
        std::set<std::string>     symbols_;
};
FORWARD_DECLARE_PTR(RspSymbolListData);

const long UT_FID_ReqRiskCtrledDepthData = 0x10004;
class ReqRiskCtrledDepthData:public Socket
{
    public:
    ReqRiskCtrledDepthData(string symbol, ID_TYPE socket_id, COMM_TYPE type):Socket(socket_id, type)
    {
        assign(symbol_, symbol);
    }

    ReqRiskCtrledDepthData(const ReqRiskCtrledDepthData& other):Socket(other.socket_id_, other.socket_type_)
    {
        assign(symbol_, other.symbol_);
        assign(is_canacel_request_, other.is_canacel_request_);
    }

    void set(string symbol, ID_TYPE socket_id, COMM_TYPE socket_type=COMM_TYPE::WEBSOCKET, bool is_canacel_request=false)
    {
        assign(symbol_, symbol);
        socket_id_ = socket_id;
        socket_type_ = socket_type;
        is_canacel_request_ = is_canacel_request;
    }

    symbol_type symbol_;
    bool                is_canacel_request_{false};

    static const long Fid = UT_FID_ReqRiskCtrledDepthData;
};
FORWARD_DECLARE_PTR(ReqRiskCtrledDepthData);

const long UT_FID_RspRiskCtrledDepthData = 0x10005;
class RspRiskCtrledDepthData:public Socket, public boost::enable_shared_from_this<RspRiskCtrledDepthData>
{
    public:
        RspRiskCtrledDepthData(const SDepthData* depth_data, ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            set(depth_data, socket_id, socket_type);
        }

        RspRiskCtrledDepthData(const RspRiskCtrledDepthData& other):Socket(other.socket_id_, other.socket_type_)
        {
            cout << "RspRiskCtrledDepthData::RspRiskCtrledDepthData " << endl;
            
            depth_data_ = other.depth_data_;
            for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
            {
                ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
                bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
            }            
        }

        RspRiskCtrledDepthData & operator=(const RspRiskCtrledDepthData& other);

        void set(const SDepthData* depth_data, ID_TYPE socket_id, COMM_TYPE socket_type);

        virtual ~RspRiskCtrledDepthData() {}

        boost::shared_ptr<RspRiskCtrledDepthData> get_object() 
        { 
            boost::shared_ptr<RspRiskCtrledDepthData> shared_this(this); 
            return shared_this;
        }

        string get_json_str();

        SDecimal ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        SDecimal bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_RspRiskCtrledDepthData;  

    private:
        std::mutex                mutex_;
};
FORWARD_DECLARE_PTR(RspRiskCtrledDepthData);

const long UT_FID_ReqEnquiry = 0x10006;
class ReqEnquiry:public Socket
{
    public:
        ReqEnquiry(string symbol, double volumn, double amount, int type, ID_TYPE socket_id, COMM_TYPE socket_type):
        Socket(socket_id, socket_type)
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

            socket_id_ = other.socket_id_;
            socket_type_ = other.socket_type_;               
        }

        void set(string symbol, double volumn, double amount, int type,
                 ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            assign(symbol_, symbol);
            assign(volume_, volumn);
            assign(amount_, amount); 
            assign(type_, type);

            socket_id_ = socket_id;
            socket_type_ = socket_type;  
        }        
    static const long Fid = UT_FID_ReqEnquiry;

        symbol_type symbol_;
        int         type_{-1};
        double      volume_{-1};
        double      amount_{-1};

    
};
FORWARD_DECLARE_PTR(ReqEnquiry);

const long UT_FID_RspEnquiry = 0x10007;
class RspEnquiry:public Socket
{
    public:
        RspEnquiry(string symbol, double price, ID_TYPE socket_id, COMM_TYPE socket_type):
        Socket(socket_id, socket_type)
        {
            assign(price_, price);
            assign(symbol_, symbol);
        }

        void set(string symbol, double price, ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            assign(price_, price);
            assign(symbol_, symbol);    

            socket_id_ = socket_id;
            socket_type_ = socket_type;                     
        }

        string get_json_str();
        
    static const long Fid = UT_FID_RspEnquiry;
    
    private:
        double          price_;
        symbol_type     symbol_;  
};
FORWARD_DECLARE_PTR(RspEnquiry);

const long UT_FID_ReqKLineData = 0x10008;
class ReqKLineData:public Socket
{
    public:
    ReqKLineData():Socket()
    {

    }

    virtual ~ReqKLineData()
    {
        cout << "~ReqKLineData() " << endl;
    }

    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
                 ID_TYPE socket_id, COMM_TYPE socket_type, bool is_canacel_request=false):
    Socket(socket_id, socket_type)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(is_canacel_request_, is_canacel_request);
    }

    ReqKLineData(const ReqKLineData& other):Socket(other.socket_id_, other.socket_type_)
    {
        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(data_count_, other.data_count_);
        assign(frequency_, other.frequency_); 
    }

    ReqKLineData& operator=(const ReqKLineData& other)
    {
        if (this == &other) return *this;

        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(frequency_, other.frequency_);
        assign(data_count_, other.data_count_);
        assign(ws_id_, other.ws_id_);

        socket_id_ = other.socket_id_;
        socket_type_ = other.socket_type_;
        return *this;        
    }

    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
                 ID_TYPE socket_id, COMM_TYPE socket_type):
    Socket(socket_id, socket_type)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
    }    

    void set(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
             ID_TYPE socket_id, COMM_TYPE socket_type, bool is_canacel_request=false)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(is_canacel_request_, is_canacel_request);

        socket_id_ = socket_id;
        socket_type_ = socket_type;      
    }

    void reset(const ReqKLineData& other)
    {
        assign(symbol_, other.symbol_);
        assign(start_time_, other.start_time_);
        assign(end_time_, other.end_time_);
        assign(frequency_, other.frequency_);
        assign(data_count_, other.data_count_);
        assign(ws_id_, other.ws_id_);
 
        socket_id_ = other.socket_id_;
        socket_type_ = other.socket_type_;
    }
  
    public: 
        symbol_type         symbol_;
        type_tick           start_time_{0};
        type_tick           end_time_{0};
        type_tick           append_end_time_{0};
        int                 data_count_{-1};

        ID_TYPE             ws_id_{-1};

        frequency_type      frequency_;
        bool                is_canacel_request_{false};

        static const long Fid = UT_FID_ReqKLineData;
};
FORWARD_DECLARE_PTR(ReqKLineData);

const long UT_FID_RspKLineData = 0x10009;
class RspKLineData:public Socket
{
    public: 

        void set(string symbol, type_tick start_time, type_tick end_time, frequency_type frequency, 
                 int data_count,  ID_TYPE socket_id, COMM_TYPE socket_type, bool is_update,
                 std::vector<KlineDataPtr>& kline_data_vec)
        {
            assign(symbol_, symbol);
            assign(start_time_, start_time);
            assign(end_time_, end_time);
            assign(frequency_, frequency);
            assign(data_count_, data_count);
            assign(is_update_, is_update);

            kline_data_vec_.swap(kline_data_vec);
        }

        void set(ReqKLineData * pReqKlineData,
                 std::vector<KlineDataPtr>& kline_data_vec)
        {
            assign(symbol_, pReqKlineData->symbol_);
            assign(start_time_, pReqKlineData->start_time_);
            assign(end_time_, pReqKlineData->end_time_);
            assign(frequency_, pReqKlineData->frequency_);
            assign(data_count_, kline_data_vec.size());
            assign(is_update_, false);

            socket_id_ = pReqKlineData->socket_id_;
            socket_type_ = pReqKlineData->socket_type_;

            kline_data_vec_.swap(kline_data_vec);
        }   

        void set(ReqKLineData * pReqKlineData,
                 KlineDataPtr& kline_data)
        {
            assign(symbol_, pReqKlineData->symbol_);
            assign(start_time_, pReqKlineData->start_time_);
            assign(end_time_, pReqKlineData->end_time_);
            assign(frequency_, pReqKlineData->frequency_);
            assign(data_count_, pReqKlineData->data_count_);
            assign(is_update_, true);

            socket_id_ = pReqKlineData->socket_id_;
            socket_type_ = pReqKlineData->socket_type_;

            kline_data_vec_.push_back(kline_data);
        }        

        symbol_type                         symbol_;
        type_tick                           start_time_;
        type_tick                           end_time_;
        frequency_type                      frequency_;

        int                                 data_count_;
        std::vector<KlineDataPtr>           kline_data_vec_;

        bool                                is_update_{false};

        static const long Fid = UT_FID_RspKLineData;

        string get_json_str();
};
FORWARD_DECLARE_PTR(RspKLineData);

const long UT_FID_RspErrorMsg = 0x100010;
class RspErrorMsg:public Socket
{
    public:
        RspErrorMsg(string err_msg, int err_id, 
                    ID_TYPE socket_id, COMM_TYPE socket_type):
        Socket(socket_id, socket_type)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);
        }

        void set(string err_msg, int err_id,  
                 ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);

            socket_id_ = socket_id;
            socket_type_ = socket_type;                                     
        }

        string get_json_str();

    static const long Fid = UT_FID_RspErrorMsg;
    
    private:
        int             err_id_;
        error_msg_type  err_msg_;
};
FORWARD_DECLARE_PTR(RspErrorMsg);