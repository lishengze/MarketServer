#pragma once

#include "stream_engine_define.h"
#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"

#include "base/cpp/quote.h"
#include "stream_engine_config.h"

using namespace std;
using namespace nacos;

class INacosCallback {
public:
    virtual void on_config_channged(const NacosString& symbols) = 0;
};

class NacosListener : public Listener 
{
private:
    string group_;
    string dataid_;
    ConfigService* server_;
    INacosCallback* callback_;
public:
    NacosListener(ConfigService* server, string group, string dataid, INacosCallback* callback) 
    {
        group_ = group;
        dataid_ = dataid;
        server_ = server;
        callback_ = callback;

        NacosString ss = "";
        try {
            ss = server_->getConfig(dataid_, group_, 1000);
        }
        catch (NacosException &e) {
            cout <<
                "Request failed with curl code:" << e.errorcode() << endl <<
                "Reason:" << e.what() << endl;
            return;
        }
        receiveConfigInfo(ss);
    }

    void receiveConfigInfo(const NacosString &configInfo) {
        if( group_ == "quotation" && dataid_ == "symbols" ) {
            _log_and_print("quotation:symbols changed. %s", configInfo.c_str());
            callback_->on_config_channged(configInfo);
        }
    }
};

class NacosClient
{
public:
    NacosClient(){}
    ~NacosClient(){}

    void start(const string& addr, INacosCallback* callback) 
    {
        _log_and_print("connect nacos addr %s.", addr.c_str());
        _run_thread_ = new std::thread(&NacosClient::_run, this, addr, callback);
    }

private:
    std::thread* _run_thread_ = nullptr;

    void _run(const string& addr, INacosCallback* callback)
    {
        Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = addr;
        props[PropertyKeyConst::NAMESPACE] = "bcts";
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        try {
            NacosListener *listener1 = new NacosListener(n, "quotation", "symbols", callback);
            n->addListener("symbols", "quotation", listener1);
        }
        catch (NacosException &e) {
            cout <<
                "Request failed with curl code:" << e.errorcode() << endl <<
                "Reason:" << e.what() << endl;
            return;
        }

        while( true ) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
