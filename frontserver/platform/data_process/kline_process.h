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

    ReqKLineData        reqkline_data;
    type_tick           last_update_time_;
    KlineDataPtr        kline_data_{nullptr};
};
FORWARD_DECLARE_PTR(KlineDataUpdate);

class TradeDataUpdate
{
    public:
    TradeDataUpdate(const ReqTrade& reqTrade)
    {
        pReqTrade_ = boost::make_shared<ReqTrade>(reqTrade);
    }    

    ReqTradePtr    pReqTrade_{nullptr};

};
FORWARD_DECLARE_PTR(TradeDataUpdate);

class KlineProcess
{
public:
    using DataProcessPtr = boost::shared_ptr<DataProcess>;

    KlineProcess();

    virtual ~KlineProcess() {}

    void init_config();

    void init_process_engine(DataProcessPtr process_engine);

    void response_src_kline_package(PackagePtr package);

    void response_src_trade_package(PackagePtr package);

    void request_kline_package(PackagePtr package);

    void request_trade_package(PackagePtr package);

    PackagePtr get_kline_package(PackagePtr package);

    bool delete_kline_request_connect(string symbol, ID_TYPE socket_id);

    bool store_kline_data(int frequency, KlineDataPtr pkline_data, int base_frequency);

    void complete_kline_data(vector<KlineData>& ori_symbol_kline_data, vector<KlineData>& append_result, frequency_type frequency);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time, int cur_freq_base);

    void get_src_kline_data(string symbol, vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, int data_count, int cur_freq_base);

    vector<KlineDataPtr> compute_target_kline_data(vector<KlineDataPtr>& kline_data, int frequency);

    void init_update_kline_data(PackagePtr rsp_package, ReqKLineDataPtr pReqKlineData);

    void update_kline_data(const KlineDataPtr kline_data);

    void check_websocket_subinfo(ReqKLineDataPtr pReqKlineData);

    void update_frequency_aggreration_map(int src_fre);

    int get_best_freq_base(int req_frequency);

    void init_update_trade_map(ReqTradePtr pReqTrade);

    void check_websocket_trade_req(ReqTradePtr pReqTrade);

    void update_trade_data(const TradeDataPtr pTradeDataPtr);

    PackagePtr get_trade_package(ReqTradePtr pReqTrade);
    
    std::map<type_tick, KlineDataPtr>& get_trade_kline_data(int freq,int start_time);

    // void get_append_data(type_tick start_time, type_tick end_time, int data_count, vector<KlineData>& append_result);

private:
    DataProcessPtr                                              process_engine_;   

    std::mutex                                                  kline_data_mutex_;
    map<string, map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;
    map<string, map<int, KlineDataPtr>>                         cur_kline_data_;

    map<string, vector<KlineDataUpdate>>                        updated_kline_data_map_;

    map<string, map<ID_TYPE, TradeDataUpdatePtr>>               updated_trade_data_map_;
    std::mutex                                                  updated_trade_data_map_mutex_;

    map<string, TradeDataPtr>                                   trade_data_map_;
    std::mutex                                                  trade_data_map_mutex_;

    std::map<ID_TYPE, string>                                   trade_wss_con_map_;  
    std::mutex                                                  trade_wss_con_map_mutex_;

    std::map<ID_TYPE, string>                                   wss_con_map_;  
    std::mutex                                                  wss_con_map_mutex_;

    std::mutex                                                  updated_kline_data_map_mutex_;

    bool                                                        test_kline_data_{false};

    int                                                         frequency_cache_numb_{1000}; 
    map<int, int>                                               frequency_aggreration_map_;
    set<int>                                                    frequency_cache_set_;
    set<int>                                                    frequency_base_set_;        
};

FORWARD_DECLARE_PTR(KlineProcess);