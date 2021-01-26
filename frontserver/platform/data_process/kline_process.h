#pragma once

#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"
using std::map;
using std::vector;

class DataProcess;

class KlineDataUpdate
{
    public:
        // KlineDataUpdate(const ReqKLineData& other)
        // {
        //     assign(symbol_, other.symbol_);
        //     assign(start_time_, other.start_time_);
        //     assign(end_time_, other.end_time_);
        //     assign(data_count_, other.data_count_);
        //     assign(frequency_, other.frequency_); 
        //     websocket_ = other.websocket_->get_ws();

        //     websocket_com_ = boost::make_shared<WebsocketClassThreadSafe>(other.websocket_->get_ws());
        //     websocket_ = other.websocket_;
        // }

        KlineDataUpdate(const ReqKLineData& other): reqkline_data{other}
        {
        }


        ~KlineDataUpdate()
        {
            cout << "~KlineDataUpdate() " << endl;
        }

    // ReqKLineData   req_kline_data_;

    // symbol_type                     symbol_{NULL};
    // type_tick                       start_time_{0};
    // type_tick                       end_time_{0};
    // int                             data_count_{-1};
    // frequency_type                  frequency_{0};   

    // WebsocketClass*                 websocket_{nullptr};

    // WebsocketClassThreadSafePtr     websocket_com_{nullptr};
    ReqKLineData        reqkline_data;
    type_tick           last_update_time_;
    KlineDataPtr        kline_data_{nullptr};
};
FORWARD_DECLARE_PTR(KlineDataUpdate);

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

    bool delete_kline_request_connect(string symbol, ID_TYPE socket_id);

    void store_kline_data(int frequency, KlineData* pkline_data);

    void complete_kline_data(vector<KlineData>& ori_symbol_kline_data, vector<KlineData>& append_result, frequency_type frequency);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, int data_count);

    vector<KlineDataPtr> compute_target_kline_data(vector<KlineDataPtr>& kline_data, int frequency);

    void init_test_kline_data();

    void init_update_kline_data(PackagePtr rsp_package, ReqKLineData * pReqKlineData);

    void update_kline_data(const KlineData* kline_data);

    void check_websocket_subinfo(ReqKLineData* pReqKlineData);

private:
    DataProcessPtr                                              process_engine_;   

    std::mutex                                                  kline_data_mutex_;
    map<string, map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;
    map<string, map<int, KlineDataPtr>>                         cur_kline_data_;

    map<string, vector<KlineDataUpdate>>                        updated_kline_data_;
    std::map<ID_TYPE, string>                                   wss_con_map_;  
    std::mutex                                                  wss_con_map_mutex_;

    std::mutex                                                  updated_kline_data_mutex_;

    bool                                                        test_kline_data_{false};

    vector<int>                                                 frequency_list_;
    int                                                         frequency_numb_{100};   
    vector<int>                                                 frequency_base_list_;        
};

FORWARD_DECLARE_PTR(KlineProcess);