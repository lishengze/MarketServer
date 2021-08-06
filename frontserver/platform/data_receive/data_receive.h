#pragma once

#include "pandora/package/package_simple.h"
#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"

#include <thread>
#include <memory>

#include "hub_interface.h"
#include "hub_struct.h"
#include "../data_structure/base_data.h"

#include "../front_server_declare.h"

// 用于处理数据: 为 front-server 提供全量或增量的更新;
// 设置缓冲;
class DataReceive:public utrade::pandora::ThreadBasePool, public IPackageStation, public HubCallback
{
public:
    DataReceive(utrade::pandora::io_service_pool& pool, IPackageStation* next_station=nullptr);
    ~DataReceive();

    void init_grpc_interface();

    virtual void launch() override;

    virtual void release() override;

    virtual void request_message(PackagePtr package) override;

    virtual void response_message(PackagePtr package) override;

    void handle_request_message(PackagePtr package);

    void handle_response_message(PackagePtr package);

    // 深度数据（推送）
    virtual int on_depth(const char* exchange, const char* symbol, const SDepthData& depth);

    // K线数据（推送）
    virtual int on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines);

    // 原始深度数据推送
    // virtual int on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth);

    // 成交
    virtual int on_trade(const char* exchange, const char* symbol, const Trade& trade);

    void handle_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth);

    void handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth);

    void handle_kline_data(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines);    

    void handle_trade_data(const char* exchange, const char* symbol, const Trade& trade);

    void test_kline_data();

    void test_enquiry();

    void test_rsp_package();

    void test_main();

    std::shared_ptr<std::thread>  test_kline_thread_{nullptr};
    std::shared_ptr<std::thread>  test_enquiry_thread_{nullptr};
    
    bool                          is_test_kline{false};
    bool                          is_test_depth{false};    
    bool                          is_test_enquiry{false};

    bool                is_test_maxmin_kline{true};
    string              test_kline_symbol{"BTC_USDT"};
    MaxMinKlineInfo     max_min_kline_info_60;
    MaxMinKlineInfo     max_min_kline_info_3600;

private:
   
};