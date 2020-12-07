#pragma once

#include "pandora/util/thread_basepool.h"
#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/package/package_manager.h"

#include "data_process/data_process.h"
#include "data_receive/data_receive.h"
#include "front_server/front_server.h"

FORWARD_DECLARE_PTR(DataProcess);
FORWARD_DECLARE_PTR(DataReceive);
FORWARD_DECLARE_PTR(FrontServer);

#define SERVER_EENGINE utrade::pandora::ThreadSafeSingleton<ServerEngine>::GetInstance()

class ServerEngine
{
    using PackageManagerPtr = boost::shared_ptr<utrade::pandora::PackageManager>;
public:
    explicit ServerEngine(utrade::pandora::io_service_pool& pool);
    ~ServerEngine();

    void launch();
    void release();

    
    PackageManagerPtr package_manager() {return package_manager_;}

    static volatile int signal_sys;
    static void signal_handler(int signum);

private:
    PackageManagerPtr   package_manager_;

    DataReceivePtr          data_receiver_;
    DataProcessPtr          data_processer_;    
    FrontServerPtr          front_server_;
};