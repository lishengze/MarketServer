#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"
#include "../util/id.hpp"
#include "base_data.h"
#include "hub_struct.h"


const long UT_FID_ReqSymbolListData = 10002;
class ReqSymbolListData:public Socket, virtual public PacakgeBaseData
{
    public:
        ReqSymbolListData(ID_TYPE socket_id, COMM_TYPE socket_type, bool is_canacel_request=false): 
                            Socket(socket_id, socket_type), is_canacel_request_(is_canacel_request)
        {

        }

        ReqSymbolListData(WebsocketClassThreadSafePtr ws, bool is_canacel_request=false): 
                            Socket(ws, ws->get_id()), is_canacel_request_(is_canacel_request)
        {

        }

        ReqSymbolListData(WebsocketClassThreadSafePtr ws, ID_TYPE socket_id,  bool is_canacel_request=false): 
                            Socket(ws, socket_id), is_canacel_request_(is_canacel_request)
        {

        }

        ReqSymbolListData(const ReqSymbolListData& other)
        {
            socket_id_ = other.socket_id_;
            socket_type_ = other.socket_type_;
            http_response_ = other.http_response_;
            websocket_ = other.websocket_;


            assign(is_canacel_request_, other.is_canacel_request_);
        }

        void set(ID_TYPE socket_id, COMM_TYPE socket_type, bool is_canacel_request=false)
        {
            socket_id_ = socket_id;
            socket_type_ = socket_type;    
            is_canacel_request_ =  is_canacel_request;
        }

        void set(WebsocketClassThreadSafePtr ws, bool is_canacel_request=false)
        {
            websocket_ = ws; 
            is_canacel_request_ =  is_canacel_request;
        }

        string str()
        {
            string req_type = is_canacel_request_? "_is_canacel_request_":"";
            return string("\nReqSymbolListData-Info: \n") + Socket::str() + req_type + "\n";
        }

        bool                is_canacel_request_{false};

        static const long Fid = UT_FID_ReqSymbolListData; 
};
FORWARD_DECLARE_PTR(ReqSymbolListData);

const long UT_FID_RspSymbolListData = 10003;
class RspSymbolListData:public Socket, virtual public PacakgeBaseData
{
    public:
        RspSymbolListData(std::set<std::string>& symbols, ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            assign(symbols_, symbols);
            socket_id_ = socket_id;
            socket_type_ = socket_type;
        }

        RspSymbolListData(std::set<std::string>& symbols, WebsocketClassThreadSafePtr ws)
        {
            assign(symbols_, symbols);
            websocket_ = ws;
            socket_id_ = ws->get_id();
        }


        RspSymbolListData(std::set<std::string>& symbols)
        {
            assign(symbols_, symbols);
        }        

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

        void set(std::set<std::string>& symbols, WebsocketClassThreadSafePtr ws)
        {
            assign(symbols_, symbols);

            websocket_ = ws;
        }        

        std::set<std::string>& get_symbols() { return symbols_;}

        string get_json_str();

        string str()
        {
            return string("\nRspSymbolListData-Info: \n") + Socket::str() + "\n" + get_json_str() + "\n";
        }

        static const long Fid = UT_FID_RspSymbolListData; 
    private:
        std::set<std::string>     symbols_;
};
FORWARD_DECLARE_PTR(RspSymbolListData);

const long UT_FID_ReqRiskCtrledDepthData = 10004;
class ReqRiskCtrledDepthData:public Socket, virtual public PacakgeBaseData
{
    public:
    ReqRiskCtrledDepthData(string symbol, ID_TYPE socket_id, COMM_TYPE socket_type=COMM_TYPE::WEBSOCKET, bool is_canacel_request=false)
                            :Socket(socket_id, socket_type), is_canacel_request_(is_canacel_request)
    {
        assign(symbol_, symbol);
    }

    ReqRiskCtrledDepthData(string symbol, WebsocketClassThreadSafePtr ws, ID_TYPE socket_id, bool is_canacel_request=false)
                            :Socket(ws, socket_id), is_canacel_request_(is_canacel_request)
    {
        assign(symbol_, symbol);
    }    

    ReqRiskCtrledDepthData(string symbol, WebsocketClassThreadSafePtr ws, bool is_canacel_request=false)
                            :Socket(ws), is_canacel_request_(is_canacel_request)
    {
        assign(symbol_, symbol);
    }    


    ReqRiskCtrledDepthData(const ReqRiskCtrledDepthData& other):
        Socket(other.socket_id_, other.socket_type_, other.http_response_, other.websocket_)
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

    void set(string symbol, WebsocketClassThreadSafePtr ws, bool is_canacel_request=false)
    {
        assign(symbol_, symbol);
        websocket_ = ws;
        is_canacel_request_ = is_canacel_request;
    }    

    symbol_type         symbol_;
    bool                is_canacel_request_{false};

    string str()
    {
        string type_str = is_canacel_request_ ? "_is_canacel_request" : "";
        string result = string("\nReqRiskCtrledDepthData-Info: \n") + Socket::str() + "_" + string(symbol_) + type_str + "\n";
        return result;
    }

    static const long Fid = UT_FID_ReqRiskCtrledDepthData;
};
FORWARD_DECLARE_PTR(ReqRiskCtrledDepthData);

const long UT_FID_RspRiskCtrledDepthData = 10005;
class RspRiskCtrledDepthData:public Socket, virtual public PacakgeBaseData, 
                             public boost::enable_shared_from_this<RspRiskCtrledDepthData>
{
    public:
        RspRiskCtrledDepthData(const SDepthData& depth_data, ID_TYPE socket_id, COMM_TYPE socket_type)
        {
            set_depth(depth_data);

            socket_id_ = socket_id;
            socket_type_ = socket_type;            
        }

        RspRiskCtrledDepthData(const SDepthData& depth_data, WebsocketClassThreadSafePtr ws)
        {
            set_depth(depth_data);

            websocket_ = ws;
        }


        RspRiskCtrledDepthData(const SDepthData& depth_data)
        {
            memcpy(&depth_data_, &depth_data, sizeof(SDepthData));

            for (int i = 0; i < depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
            {
                ask_accumulated_volume_[i] = i==0 ? depth_data_.asks[i].volume.get_value() : depth_data_.asks[i].volume + ask_accumulated_volume_[i-1];
            }

            for (int i = 0; i < depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
            {
                bid_accumulated_volume_[i] = i==0 ? depth_data_.bids[i].volume.get_value() : depth_data_.bids[i].volume + bid_accumulated_volume_[i-1];
            }
        }

        RspRiskCtrledDepthData(const RspRiskCtrledDepthData& other):
            Socket(other.socket_id_, other.socket_type_, other.http_response_, other.websocket_)
        {            
            depth_data_ = other.depth_data_;
            for (int i = 0; i < DEPCH_LEVEL_COUNT; ++i)
            {
                ask_accumulated_volume_[i] = other.ask_accumulated_volume_[i];
                bid_accumulated_volume_[i] = other.bid_accumulated_volume_[i];
            }            
        }

        RspRiskCtrledDepthData & operator=(const RspRiskCtrledDepthData& other);

        void set(const SDepthData& depth_data, ID_TYPE socket_id, COMM_TYPE socket_type);

        void set_depth(const SDepthData& depth_data);

        virtual ~RspRiskCtrledDepthData() {}

        boost::shared_ptr<RspRiskCtrledDepthData> get_object() 
        { 
            boost::shared_ptr<RspRiskCtrledDepthData> shared_this(this); 
            return shared_this;
        }

        string get_json_str();

        string str()
        {
            return string("\nRspRiskCtrledDepthData-Info: \n") + Socket::str() + "\n" + get_json_str() + "\n";
        }

        string simple_str()
        {
            return string(depth_data_.exchange) + "." + string(depth_data_.symbol) 
                    + ", ask.size: " + std::to_string(depth_data_.ask_length)
                    + ", bid.size: " + std::to_string(depth_data_.bid_length);
        }

        SDecimal ask_accumulated_volume_[DEPCH_LEVEL_COUNT];
        SDecimal bid_accumulated_volume_[DEPCH_LEVEL_COUNT];

        SDepthData depth_data_;

        static const long Fid = UT_FID_RspRiskCtrledDepthData;  

    private:
        std::mutex                mutex_;
};
FORWARD_DECLARE_PTR(RspRiskCtrledDepthData);

const long UT_FID_ReqEnquiry = 10006;
class ReqEnquiry:public Socket, virtual public PacakgeBaseData
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

const long UT_FID_RspEnquiry = 10007;
class RspEnquiry:public Socket, virtual public PacakgeBaseData
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

const long UT_FID_ReqKLineData = 10008;
class ReqKLineData:public Socket, virtual public PacakgeBaseData
{
    public:
    ReqKLineData():Socket()
    {

    }

    virtual ~ReqKLineData()
    {
        // cout << "~ReqKLineData() " << endl;
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

    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
                 WebsocketClassThreadSafePtr ws, bool is_canacel_request=false):
                 Socket(ws, ws->get_id())
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(is_canacel_request_, is_canacel_request);
    }   

    ReqKLineData(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
                 WebsocketClassThreadSafePtr ws, ID_TYPE socket_id, bool is_canacel_request=false):
                 Socket(ws, socket_id)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(is_canacel_request_, is_canacel_request);
    }    

    ReqKLineData(const ReqKLineData& other)
    :Socket(other.socket_id_, other.socket_type_, other.http_response_, other.websocket_)
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
        http_response_ = other.http_response_;
        websocket_ = other.websocket_; 
        return *this;        
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

    void set(string symbol, type_tick start_time, type_tick end_time, int data_count, int freq, 
             WebsocketClassThreadSafePtr ws, bool is_canacel_request=false)
    {
        assign(symbol_, symbol);
        assign(start_time_, start_time);
        assign(end_time_, end_time);
        assign(data_count_, data_count);
        assign(frequency_, freq);
        assign(is_canacel_request_, is_canacel_request);

        websocket_ = ws;
        socket_type_ = COMM_TYPE::WEBSOCKET;
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
        http_response_ = other.http_response_;
        websocket_ = other.websocket_; 
    }

    string str()
    {
        std::stringstream req_stream;
        req_stream << "symbol: " <<  symbol_ << "\n"
                   << "start_time_: " << start_time_ << "\n"
                   << "end_time_: " << end_time_ << "\n"
                   << "append_end_time_: " << append_end_time_ << "\n"
                   << "data_count_: " << data_count_ << "\n"
                   << "frequency_: " << frequency_ << "\n"
                   << "is_canacel_request_: " << is_canacel_request_ << "\n";

        return string("\nReqKLineData-Info: \n") + Socket::str() + "\n" + req_stream.str() + "\n";
    }

    string simple_str()
    {
        std::stringstream req_stream;
        req_stream << "[depth] symbol: " <<  symbol_ << "\n"
                   << "socket_id: " << socket_id_ << "\n"
                   << "frequency_: " << frequency_ << "\n";

        return req_stream.str();
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

const long UT_FID_RspKLineData = 10009;
class RspKLineData:public Socket, virtual public PacakgeBaseData
{
    public: 
        RspKLineData(ReqKLineData& pReqKlineData,
                    std::vector<KlineDataPtr>& kline_data_vec)
        {
            assign(symbol_, pReqKlineData.symbol_);
            assign(start_time_, pReqKlineData.start_time_);
            assign(end_time_, pReqKlineData.end_time_);
            assign(frequency_, pReqKlineData.frequency_);
            assign(data_count_, kline_data_vec.size());
            assign(is_update_, false);

            socket_id_ = pReqKlineData.socket_id_;
            socket_type_ = pReqKlineData.socket_type_;
            http_response_ = pReqKlineData.http_response_;
            websocket_ = pReqKlineData.websocket_;

            kline_data_vec_.swap(kline_data_vec);
        }   

        RspKLineData(ReqKLineData& pReqKlineData,
                    KlineDataPtr& kline_data)
        {
            assign(symbol_, pReqKlineData.symbol_);
            assign(start_time_, pReqKlineData.start_time_);
            assign(end_time_, pReqKlineData.end_time_);
            assign(frequency_, pReqKlineData.frequency_);
            assign(data_count_, pReqKlineData.data_count_);
            assign(is_update_, true);

            socket_id_ = pReqKlineData.socket_id_;
            socket_type_ = pReqKlineData.socket_type_;
            http_response_ = pReqKlineData.http_response_;
            websocket_ = pReqKlineData.websocket_;

            kline_data_vec_.push_back(kline_data);
        }     


        RspKLineData(KlineDataPtr& kline_data)
        {
            assign(symbol_, kline_data->symbol);
            assign(frequency_, kline_data->frequency_);
            assign(data_count_, 1);
            assign(is_update_, true);

            socket_id_ = -1;
            kline_data_vec_.push_back(kline_data);
        }         

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

        void set(string symbol, type_tick start_time, type_tick end_time, frequency_type frequency, 
                 int data_count,  WebsocketClassThreadSafePtr ws, bool is_update,
                 std::vector<KlineDataPtr>& kline_data_vec)
        {
            assign(symbol_, symbol);
            assign(start_time_, start_time);
            assign(end_time_, end_time);
            assign(frequency_, frequency);
            assign(data_count_, data_count);
            assign(is_update_, is_update);

            websocket_ = ws;
            socket_type_ = COMM_TYPE::WEBSOCKET;
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
            http_response_ = pReqKlineData->http_response_;
            websocket_ = pReqKlineData->websocket_;


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
            http_response_ = pReqKlineData->http_response_;
            websocket_ = pReqKlineData->websocket_;

            kline_data_vec_.push_back(kline_data);
        }        

        string simple_str()
        {
            string type = is_update_? " update ":" init ";
            return string("[kline] ") + symbol_ + ", " + type 
                    + ", fre: " + std::to_string(frequency_)  
                    +", data.size: " + std::to_string(kline_data_vec_.size());
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

        string str()
        {
            return string("\nRspKLineData-Info: \n") + Socket::str() + "\n" + get_json_str() + "\n";
        }        
};
FORWARD_DECLARE_PTR(RspKLineData);

const long UT_FID_RspErrorMsg = 100010;
class RspErrorMsg:public Socket, virtual public PacakgeBaseData
{
    public:
        RspErrorMsg(string err_msg, int err_id, 
                    ID_TYPE socket_id, COMM_TYPE socket_type):
        Socket(socket_id, socket_type)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);
        }

        RspErrorMsg(string err_msg, int err_id, 
                    WebsocketClassThreadSafePtr  ws):
        Socket(ws, ws->get_id())
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

        void set(string err_msg, int err_id,  
                 WebsocketClassThreadSafePtr  ws)
        {
            assign(err_msg_, err_msg);
            assign(err_id_, err_id);

            websocket_ = ws;
            socket_type_ = COMM_TYPE::WEBSOCKET;                                  
        }        

        string get_json_str();

        string str()
        {
            return string("\nRspErrorMsg-Info: \n") + Socket::str() + "\n" + get_json_str() + "\n";
        }

    static const long Fid = UT_FID_RspErrorMsg;
    
    private:
        int             err_id_;
        error_msg_type  err_msg_;
};
FORWARD_DECLARE_PTR(RspErrorMsg);

const long UT_FID_TradeData = 100011;
struct TradeData:virtual public PacakgeBaseData
{
    TradeData(string symbol, string exchange, type_tick time, SDecimal price, SDecimal volume):
    time_{time}, price_{price}, volume_{volume}
    {
        assign(symbol_, symbol);
        assign(exchange_, exchange);
    }

    symbol_type symbol_;
    symbol_type exchange_;
    type_tick time_;    
    SDecimal price_;
    SDecimal volume_;

    type_tick   start_time_;
    SDecimal    start_price_;
    SDecimal    total_volume_;
    double      change_;
    double      change_rate_;
    SDecimal    high_;
    SDecimal    low_;

    virtual ~TradeData() {}
};
FORWARD_DECLARE_PTR(TradeData);

const long UT_FID_ReqTrade = 100012;
class ReqTrade:public Socket, virtual public PacakgeBaseData
{
public:
    ReqTrade(string symbol, bool is_cancel, ID_TYPE socket_id, COMM_TYPE socket_type):
        Socket(socket_id, socket_type)
    {
        assign(symbol_, symbol);
        assign(is_cancel_, is_cancel);
    }

    ReqTrade(string symbol, bool is_cancel, WebsocketClassThreadSafePtr ws, ID_TYPE socket_id):
            Socket(ws, socket_id)
    {
        assign(symbol_, symbol);
        assign(is_cancel_, is_cancel);
    }

    ReqTrade(string symbol, bool is_cancel, WebsocketClassThreadSafePtr ws):
            Socket(ws, ws->get_id())
    {
        assign(symbol_, symbol);
        assign(is_cancel_, is_cancel);
    }


    ReqTrade(const ReqTrade& other)
        :Socket(other.socket_id_, other.socket_type_, other.http_response_, other.websocket_)
    {
        assign(symbol_, other.symbol_);
        assign(is_cancel_, other.is_cancel_);
    }

    static const long Fid = UT_FID_ReqTrade;

    virtual ~ReqTrade() {}

    string str()
    {
        return string("\nReqTrade-Info: \n") + Socket::str() + "_" +  symbol_ + "_is_cancel_: " + std::to_string(is_cancel_) + "\n";
    }

    symbol_type symbol_;
    bool        is_cancel_{false};
};
FORWARD_DECLARE_PTR(ReqTrade);

const long UT_FID_RspTrade = 100013;
class RspTrade:public Socket, virtual public PacakgeBaseData
{
public:
    RspTrade(string symbol, SDecimal price, SDecimal volume, 
             double change, double change_rate,
             SDecimal high, SDecimal low, 
             ID_TYPE socket_id, COMM_TYPE socket_type):
             Socket(socket_id, socket_type)
    {
        assign(symbol_, symbol);
        assign(price_, price);
        assign(volume_, volume);
        assign(change_, change);
        assign(change_rate_, change_rate);
        assign(high_, high);
        assign(low_, low);
    }     

    RspTrade(string symbol, SDecimal price, SDecimal volume, 
             double change, double change_rate,
             SDecimal high, SDecimal low, 
             WebsocketClassThreadSafePtr ws):
             Socket(ws)
    {
        assign(symbol_, symbol);
        assign(price_, price);
        assign(volume_, volume);
        assign(change_, change);
        assign(change_rate_, change_rate);
        assign(high_, high);
        assign(low_, low);
    }        

    RspTrade(string symbol, SDecimal price, SDecimal volume, 
             double change, double change_rate,
             SDecimal high, SDecimal low)
    {
        assign(symbol_, symbol);
        assign(price_, price);
        assign(volume_, volume);
        assign(change_, change);
        assign(change_rate_, change_rate);
        assign(high_, high);
        assign(low_, low);
    }        

    virtual ~RspTrade() {}

    string get_json_str();

    symbol_type symbol_;
    SDecimal    price_;
    SDecimal    volume_;
    double      change_;
    double      change_rate_;
    SDecimal    high_;
    SDecimal    low_;

    string str()
    {
        return string("\nRspTrade-Info: \n") +Socket::str() + "\n" + get_json_str() + "\n";
    }

    string simple_str()
    {
        return string("[trade] ") + symbol_ 
            + ", price: " + price_.get_str_value()
            + ", volume: " + volume_.get_str_value()
            + ", change: " + std::to_string(change_)
            + ", change_rate_: " + std::to_string(change_rate_)
            + ", high: " + high_.get_str_value()
            + ", low: " + low_.get_str_value();
    }

    static const long Fid = UT_FID_RspTrade;
};
FORWARD_DECLARE_PTR(RspTrade);