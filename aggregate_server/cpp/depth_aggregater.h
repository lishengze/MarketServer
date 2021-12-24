#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"
#include "struct_define.h"

class DepthAggregater:public bcts::comm::QuoteSourceCallbackInterface
{
public:

    DepthAggregater();
    ~DepthAggregater();

    void start();

    void set_comm(bcts::comm::Comm*  comm){ p_comm_ = comm;}

    void set_config(unordered_map<TSymbol, SMixerConfig>& new_config);

    bool is_data_too_fast_or_init(TSymbol symbol, int standard_fre);

    virtual void on_snap( SDepthQuote& quote);

private:

    // 缓存数据
    void _calc_symbol(const TSymbol& symbol, const SMixerConfig& config, type_seqno seqno); 

    void _thread_loop();

private:

    bcts::comm::Comm*                                               p_comm_{nullptr};

    // 计算线程
    std::thread*                                                    thread_loop_{nullptr};
    std::atomic<bool>                                               thread_run_;

    // 配置信息
    mutable std::mutex                                              mutex_config_;
    unordered_map<TSymbol, SMixerConfig>                            configs_; 

    // 控制频率    
    unordered_map<TSymbol, type_tick>                               last_clocks_;

    mutable std::mutex                                              mutex_quotes_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>>   quotes_;
};
