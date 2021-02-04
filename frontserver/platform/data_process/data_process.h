#pragma once

#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"

#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"
#include "kline_process.h"
#include "depth_process.h"

// 用于处理数据: 为 front-server 提供全量或增量的更新;
// 设置缓冲;
class DataProcess:public utrade::pandora::ThreadBasePool, public IPackageStation, public boost::enable_shared_from_this<DataProcess>
{
public:

    DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station=nullptr);
    virtual ~DataProcess();

    virtual void launch() override;
    virtual void release() override;
            
    virtual void request_message(PackagePtr package) override;
    virtual void response_message(PackagePtr package) override;

    boost::shared_ptr<DataProcess> get_shared_ptr() { return shared_from_this();}

    void handle_request_message(PackagePtr package);

    void handle_response_message(PackagePtr package);

    void request_kline_package(PackagePtr package);

    void request_trade_package(PackagePtr package);

    void request_depth_package(PackagePtr package);

    void request_symbol_list_package(PackagePtr package);

    void request_enquiry_package(PackagePtr package);

    void response_src_sdepth_package(PackagePtr package);    

    void response_src_kline_package(PackagePtr package);

    void response_src_trade_package(PackagePtr package);

private:
    KlineProcessPtr         kline_process_{nullptr};
    DepthProcesPtr          depth_process_{nullptr};
             
};

FORWARD_DECLARE_PTR(DataProcess);

