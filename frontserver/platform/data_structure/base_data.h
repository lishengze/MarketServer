#pragma once

#include <mutex>
#include <boost/shared_ptr.hpp>

#include "../front_server_declare.h"
#include "../util/id.hpp"
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
    bool                   is_alive{false};
    // std::set<symbol_type>  sub_symbol_set;
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
        // cout << "ws send: " << msg << endl;
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

    // void clear_sub_symbol_list()
    // {
    //     std::lock_guard<std::mutex> lk(mutex_);
    //     user_data_.sub_symbol_set.clear();
    // }

    // void add_sub_symbol(std::string symbol)
    // {
    //     std::lock_guard<std::mutex> lk(mutex_);
    //     user_data_.sub_symbol_set.emplace(symbol);
    // }

    // bool is_symbol_subed(std::string symbol)
    // {
    //     std::lock_guard<std::mutex> lk(mutex_);

    //     if (user_data_.sub_symbol_set.find(symbol) != user_data_.sub_symbol_set.end())
    //     {
    //         return true;
    //     }
    //     else
    //     {
    //         return false;
    //     }
    // }

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
            // cout << "IS COMM_TYPE::WEBSOCKET" << endl;
            websocket_ = boost::make_shared<WebsocketClassThreadSafe>(ws);
            comm_type = COMM_TYPE::WEBSOCKET;
        }

        if (res)
        {
            // cout << "IS COMM_TYPE::HTTP" << endl;
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

    virtual ~Socket()
    {
        cout << "~Socket() " << endl;
    }

    COMM_TYPE                       comm_type = COMM_TYPE::HTTP;
    HttpResponseThreadSafePtr       http_response_{nullptr};
    WebsocketClassThreadSafePtr     websocket_{nullptr};
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