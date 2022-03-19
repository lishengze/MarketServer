#pragma once

#include <iostream>
#include <thread>
#include <atomic>

#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"


using namespace std;
using namespace nacos;

class NacosClient;
class NacosListener : public Listener 
{
private:
    string group_;
    string dataid_;
    NacosClient* parent_ = nullptr;
public:
    void init(ConfigService* service, string group, string dataid, NacosClient* parent);

    // derive from Listener
    void receiveConfigInfo(const NacosString &configInfo);
};

class NacosClient
{
public:
    void start(const string& addr, const string& ns);

    bool is_running() { return thread_run_; }

    virtual void config_changed(const string& group, const string& dataid, const NacosString &configInfo) = 0;

    void add_listener(const string& group, const string& dataid, NacosListener& listener);
private:
    virtual void _run() = 0;

    std::thread* _run_thread_ = nullptr;
    std::atomic<bool> thread_run_;
    ConfigService* service_ = nullptr;
};
