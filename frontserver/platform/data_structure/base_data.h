#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"
#include "../util/id.hpp"
#include "hub_struct.h"
#include "pandora/package/package_simple.h"

enum class COMM_TYPE {
    HTTP = 0,
    HTTPS,
    WEBSOCKET,
    WEBSECKETS,
};

class HttpRequestThreadSafe:virtual public PacakgeBaseData
{
    public:

    HttpRequest*    http_request_{nullptr};
    std::mutex      mutex_;
};
FORWARD_DECLARE_PTR(HttpRequestThreadSafe);

class HttpResponseThreadSafe:virtual public PacakgeBaseData
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
    ID_TYPE                id_{0};
    // std::mutex             mutex_;

    ID_TYPE get_id() 
    { 
        // std::lock_guard<std::mutex> lk(mutex_);
        return id_;
    }

    void set_id(ID_TYPE id) 
    {
        // std::lock_guard<std::mutex> lk(mutex_);
        id_ = id;
    }
};

class WebsocketClassThreadSafe:virtual public PacakgeBaseData
{
    public:

    WebsocketClassThreadSafe(WebsocketClass* ws, ID_TYPE id): id_{id}
    {
        ws_ = ws;
    }

    void send(const string& msg)
    {
        std::lock_guard<std::mutex> lk(mutex);
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
        return is_alive_;
    }

    void set_alive(bool value)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        is_alive_ = value;
    }

    ID_TYPE get_id()
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return id_;
    }

    WebsocketClass* get_ws() { return ws_;}


    private:
        bool                   is_alive_{false};
        WebsocketClass*        ws_{nullptr};
        std::mutex             mutex_;
        ID_TYPE                id_;
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

// struct SocketCom
// {
//     SocketCom(HttpResponse* res):socket_type{COMM_TYPE::HTTP}
//     {
//         http_response_ = boost::make_shared<HttpResponseThreadSafe>(res);
//     }

//     SocketCom(HttpResponseThreadSafePtr res):socket_type{COMM_TYPE::HTTP}
//     {
//         http_response_ = res;
//     }    

//     SocketCom(WebsocketClass* ws):socket_type{COMM_TYPE::WEBSOCKET}
//     {
//         websocket_ = boost::make_shared<WebsocketClassThreadSafe>(wsd);
//     }

//     SocketCom(WebsocketClassThreadSafePtr ws):socket_type{COMM_TYPE::WEBSOCKET}
//     {
//         websocket_ = ws;
//     }    

//     SocketCom(HttpResponse* res=nullptr, WebsocketClass* ws=nullptr)
//     {
//         if (ws)
//         {
//             // cout << "IS COMM_TYPE::WEBSOCKET" << endl;
//             websocket_ = boost::make_shared<WebsocketClassThreadSafe>(ws);
//             socket_type = COMM_TYPE::WEBSOCKET;
//         }

//         if (res)
//         {
//             // cout << "IS COMM_TYPE::HTTP" << endl;
//             http_response_ = boost::make_shared<HttpResponseThreadSafe>(res);
//             socket_type = COMM_TYPE::HTTP;
//         }
//     }

//     SocketCom(HttpResponseThreadSafePtr res=nullptr, WebsocketClassThreadSafePtr ws=nullptr)
//     {
//         if (ws)
//         {
//             cout << "IS COMM_TYPE::WEBSOCKET" << endl;
//             websocket_ = ws;
//             socket_type = COMM_TYPE::WEBSOCKET;
//         }

//         if (res)
//         {
//             cout << "IS COMM_TYPE::HTTP" << endl;
//             http_response_ = res;
//             socket_type = COMM_TYPE::HTTP;
//         }
//     }    

//     virtual ~SocketCom()
//     {
//         cout << "~SocketCom() " << endl;
//     }

//     COMM_TYPE                       socket_type = COMM_TYPE::HTTP;
//     HttpResponseThreadSafePtr       http_response_{nullptr};
//     WebsocketClassThreadSafePtr     websocket_{nullptr};
// };

struct Socket:virtual public PacakgeBaseData
{
    Socket() {}
    Socket(ID_TYPE id, COMM_TYPE type):socket_type_{type}, socket_id_{id}
    {

    }

    COMM_TYPE                       socket_type_ = COMM_TYPE::HTTP;
    ID_TYPE                         socket_id_;
};


struct AtomKlineData:virtual public PacakgeBaseData
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