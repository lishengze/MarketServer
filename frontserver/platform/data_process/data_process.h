#pragma once

#include "pandora/package/package_station.h"
#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"
#include "../front_server_declare.h"

#include <boost/shared_ptr.hpp>
#include "../data_structure/data_struct.h"

// 用于处理数据: 为 front-server 提供全量或增量的更新;
// 设置缓冲;
class DataProcess:public utrade::pandora::ThreadBasePool, public IPackageStation
{
public:

    DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station=nullptr);
    virtual ~DataProcess();

    virtual void launch() override;
    virtual void release() override;
            
    virtual void request_message(PackagePtr package) override;
    virtual void response_message(PackagePtr package) override;

    void handle_request_message(PackagePtr package);
    void handle_response_message(PackagePtr package);

    void response_src_sdepth_package(PackagePtr package);

    void response_src_kline_package(PackagePtr package);

    void response_new_symbol(string symbol);

    void request_symbol_data(PackagePtr package);

    void request_kline_package(PackagePtr package);

    void request_depth_data(PackagePtr package);

    PackagePtr get_kline_package(PackagePtr package);

    void store_kline_data(int frequency, KlineData* pkline_data);

    void init_test_kline_data();

    void complete_kline_data(std::vector<KlineData>& ori_symbol_kline_data, std::vector<KlineData>& append_result, frequency_type frequency);

    std::vector<KlineDataPtr> get_src_kline_data(std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time);

    std::vector<AtomKlineDataPtr> compute_target_kline_data(std::vector<KlineDataPtr>& kline_data, int frequency);

    using EnhancedDepthDataPackagePtr = PackagePtr; 

private:
    std::map<string, EnhancedDepthDataPackagePtr>                         depth_data_;
    std::map<string, std::map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;
    std::map<string, std::map<int, KlineDataPtr>>                         cur_kline_data_;

    bool                                                            test_kline_data_{false};

    std::vector<int>                                                frequency_list_;
    int                                                             frequency_numb_{100};   
    int                                                             frequency_base_;           
};

