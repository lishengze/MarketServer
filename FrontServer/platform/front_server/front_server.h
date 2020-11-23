#pragma once

#include "pandora/package/package_station.h"
#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"

#include "wb_server.h"


// 用于处理数据: 为 front-server 提供全量或增量的更新;
// 设置缓冲;
class FrontServer: public utrade::pandora::ThreadBasePool, public IPackageStation
{
public:    
    FrontServer(utrade::pandora::io_service_pool& pool, IPackageStation* next_station=nullptr);
    ~FrontServer();

    virtual void launch() override;
    virtual void release() override;

    virtual void request_message(PackagePtr package) override;
    virtual void response_message(PackagePtr package) override;

    void handle_request_message(PackagePtr package);
    void handle_response_message(PackagePtr package);

private:
    WBServerPtr         wb_server_;    
};