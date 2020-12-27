#pragma once

#include <iostream>
#include <thread>

#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"

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
    NacosListener(ConfigService* server, string group, string dataid, INacosCallback* callback);

    void on_get_config(const NacosString &configInfo) const;

    void receiveConfigInfo(const NacosString &configInfo);
};

class NacosClient
{
public:
    NacosClient(){}
    ~NacosClient(){}

    void start(const string& addr, INacosCallback* callback);

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
