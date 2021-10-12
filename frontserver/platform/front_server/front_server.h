#pragma once

#include "pandora/package/package_simple.h"
#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"

#include "../front_server_declare.h"
#include "wb_server.h"
#include "rest_server.h"
#include "../data_structure/comm_data.h"


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

    void request_all_symbol();

    void process_rtn_depth_package(PackagePtr package);

    void response_sdepth_package(PackagePtr package);

    void response_kline_data_package(PackagePtr package);

    void response_trade_data_package(PackagePtr package);

    void response_enquiry_data_package(PackagePtr package);

    void response_errmsg_package(PackagePtr package);

    void response_symbol_list_package(PackagePtr package);

    void response_depth_data_package(PackagePtr package);

    std::set<std::string>& get_symbols() { return symbols_;}

    string get_symbols_str();

    string get_heartbeat_str();

public:
    void add_sub_kline(ReqKLineDataPtr req_kline_data);

private:
    WBServerPtr         wb_server_;    

    RestServerPtr       rest_server_;

    std::set<std::string>   symbols_;

private:
    map<string, map<int, std::vector<ReqKLineDataPtr>>>     sub_kline_map_;
};