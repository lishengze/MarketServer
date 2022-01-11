#pragma once

#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"
#include "../util/tools.h"
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

        string str()
        {
            std::stringstream s_obj;
            s_obj << "reqkline_data info " << "\n";
            s_obj << reqkline_data.symbol_ << "." << reqkline_data.frequency_ << ",count: " << reqkline_data.data_count_ << "\n";
            s_obj << "last kline info " << "\n";
            s_obj << get_sec_time_str(kline_data_->index)  << " " << kline_data_->symbol << "." << kline_data_->frequency_ << ", "
                        << "open: " << kline_data_->px_open.get_value() << ", high: " << kline_data_->px_high.get_value() << ", "
                        << "low: " << kline_data_->px_low.get_value() << ", close: " << kline_data_->px_close.get_value() << "\n";
            return s_obj.str();
        }


    ReqKLineData        reqkline_data;
    type_tick           last_update_time_;
    KlineDataPtr        kline_data_{nullptr};
};
FORWARD_DECLARE_PTR(KlineDataUpdate);

class ReverseCompare
{
public:
    bool operator () (const type_tick& p1, const type_tick& p2) const
    {
        return p1 > p2;
    }
};


class KlineProcess;

class TimeKlineData
{
    public:
        TimeKlineData() { }


        TimeKlineData(int freq, int last_secs, string symbol, KlineProcess* kline_process):
        frequency_{freq}, last_secs_{last_secs}, symbol_{symbol}, kline_process_{kline_process}
        {

        }

        void refresh_high_low();

        void update(KlineDataPtr kline_data);

        bool is_full();

        bool is_empty() { return ori_data_.size() == 0;}

        bool is_time_legal(type_tick time);

        void erase_out_date_data();

        std::map<type_tick, KlineDataPtr>   ori_data_;
        int                                 frequency_{0};
        int                                 last_secs_{0};

        string                              symbol_;
        SDecimal                            start_price_;

        SDecimal                            high_;
        SDecimal                            low_; 

        type_tick                           high_time_;
        type_tick                           low_time_;

        KlineProcess*                       kline_process_;

        int                                 wait_times_{0};
        std::mutex                          update_mutex_;

};
FORWARD_DECLARE_PTR(TimeKlineData);

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

    void response_kline_package(PackagePtr package);

    void response_src_trade_package(PackagePtr package);

    void request_kline_package(PackagePtr package);

    void request_trade_package(PackagePtr package);

    PackagePtr get_request_kline_package(PackagePtr package);

    bool delete_sub_kline(string symbol, int frequency);

    bool store_kline_data(int frequency, KlineDataPtr pkline_data, int base_frequency);

    void get_src_kline_data(vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, type_tick start_time, type_tick end_time, int cur_freq_base);

    void get_src_kline_data(string symbol, vector<KlineDataPtr>& result, std::map<type_tick, KlineDataPtr>& symbol_kline_data, int data_count, int cur_freq_base);

    KlineDataPtr get_last_kline_data(vector<KlineDataPtr>& kline_data, int frequency);

    vector<KlineDataPtr> compute_target_kline_data(vector<KlineDataPtr>& kline_data, int frequency);

    vector<KlineDataPtr> compute_kline_atom_data(vector<KlineDataPtr>& kline_data, int frequency);

    void init_subed_update_kline_data(PackagePtr rsp_package, ReqKLineDataPtr pReqKlineData);

    void update_subed_kline_data(const KlineDataPtr kline_data);

    void update_oneday_kline_data(const KlineDataPtr kline_data);

    void update_frequency_aggreration_map(int src_fre);

    int get_best_freq_base(int req_frequency);

    void init_update_trade_map(ReqTradePtr pReqTrade);

    void delete_sub_trade(string symbol);

    void update_trade_data(TradeDataPtr pTradeDataPtr);


    // bool need_compute_new_trade(TradeDataPtr curTradeDataPtr, TradeDataPtr oldTradeDataPtr);

    // void compute_new_trade(TradeDataPtr curTradeDataPtr);
    
    // PackagePtr get_trade_package(ReqTradePtr pReqTrade);

    // PackagePtr get_trade_package(ReqTradePtr pReqTrade, TradeDataPtr pTradeDataPtr);
    
    // std::vector<KlineDataPtr> get_trade_kline_data(string symbol, int freq_base, int start_time, int end_time);

private:
    DataProcessPtr                                              process_engine_;   

    std::mutex                                                  kline_data_mutex_;
    map<string, map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;

    map<string, TimeKlineDataPtr>                               oneday_updated_kline_data_;
    map<string, map<int, KlineDataPtr>>                         cur_kline_data_;

    map<string, map<int, KlineDataPtr>>                         sub_updated_kline_map_;
    std::mutex                                                  sub_updated_kline_map_mutex_; 

    
    set<string>                                                 sub_updated_trade_set_;
    std::mutex                                                  sub_updated_trade_set_mutex_;

    map<string, TradeDataPtr>                                   trade_data_map_;
    std::mutex                                                  trade_data_map_mutex_;

    int                                                         trade_data_freq_base_{60};

    bool                                                        test_kline_data_{false};

    int                                                         frequency_cache_numb_{1000}; 
    map<int, int>                                               frequency_aggreration_map_;
    set<int>                                                    frequency_cache_set_;
    set<int>                                                    frequency_base_set_;        
};

FORWARD_DECLARE_PTR(KlineProcess);