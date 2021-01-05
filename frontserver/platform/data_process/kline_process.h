#pragma once

#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"
using std::map;
using std::vector;

class DataProcess;
class KlineProcess
{
public:
    using DataProcessPtr = boost::shared_ptr<DataProcess>;

    KlineProcess();

    virtual ~KlineProcess() {}

    void init_config();

    void init_process_engine(DataProcessPtr process_engine);

    void response_src_kline_package(PackagePtr package);

    void request_kline_package(PackagePtr package);

    PackagePtr get_kline_package(PackagePtr package);

    void store_kline_data(int frequency, KlineData* pkline_data);

    void complete_kline_data(vector<KlineData>& ori_symbol_kline_data, vector<KlineData>& append_result, frequency_type frequency);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, int data_count);

    vector<AtomKlineDataPtr> compute_target_kline_data(vector<KlineDataPtr>& kline_data, int frequency);

    void init_test_kline_data();

private:
    DataProcessPtr                                              process_engine_;   

    std::mutex                                                  kline_data_mutex_;
    map<string, map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;
    map<string, map<int, KlineDataPtr>>                         cur_kline_data_;

    bool                                                        test_kline_data_{false};

    vector<int>                                                 frequency_list_;
    int                                                         frequency_numb_{100};   
    int                                                         frequency_base_;        
};

FORWARD_DECLARE_PTR(KlineProcess);