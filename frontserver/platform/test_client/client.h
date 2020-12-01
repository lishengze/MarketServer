#pragma once

#include "asio_httpclient/src/asio_websk.h"
#include "pandora/util/json.hpp"
#include "pandora/util/timestamp.h"
#include <set>
#include <thread>
#include <chrono>
#include <string>

#include <iostream>
using std::cout;
using std::endl;
using std::string;

// Generate Different WebSocket Connection Based on Different Account_Type;
class TestWebskClient : public asiowebsk::Client
{
public:
    TestWebskClient(string wss_address="ws://localhost:9000");

    ~TestWebskClient();

    void launch();
    void wss_run();
    void wss_connect();

    // callbacks
    virtual void OnConnected() 
    {
        cout << "OnConnected" << endl;

        std::this_thread::sleep_for(std::chrono::seconds(5));

        string send_msg = "Hello Server ";

        Send(send_msg.c_str(), send_msg.length());
    }

    virtual void OnData(string ori_data) override
    {
        cout << "ori_data: " << ori_data << endl;
    }

    virtual void OnJsonData(const char* data, size_t size) override
    {

    }

    virtual void OnError(asiowebsk::Error err_code,
                         const boost::system::error_code& boost_ec) override
    {
        cout << "Error: " << boost_ec.message().c_str() << endl;
    }                         

    virtual void OnClosed(const boost::system::error_code& boost_ec) override
    {
        cout << "Closed " << endl;
    }

private:
    string                      wss_address_{""};    
    boost::asio::io_service     ws_io_service_;
    std::thread                 thread_wss_;
};
