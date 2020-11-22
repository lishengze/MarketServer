#include "client.h"

#include <iostream>
using std::cout;
using std::endl;

TestWebskClient::TestWebskClient(string wss_address):
                    wss_address_{wss_address},
                    asiowebsk::Client(ws_io_service_)
{
    thread_wss_ = std::thread{&TestWebskClient::wss_run, this};
}

TestWebskClient::~TestWebskClient()
{
    if (thread_wss_.joinable())
    {
        thread_wss_.join();
    }
}

void TestWebskClient::launch()
{
    wss_connect();
}

void TestWebskClient::wss_run()
{
    boost::asio::io_service::work work(ws_io_service_);
    ws_io_service_.run();
}

void TestWebskClient::wss_connect()
{
    std::cout << "\n\nwss_connect " << wss_address_ << std::endl;

    auto er = SetWsAddress(wss_address_);
    if (er != asiowebsk::Success) 
    {
        cout << "asiowebsk connect failed!" << endl;
    }
}