#pragma once

#include "struct_define.h"
#include "interface_define.h"
#include "global_declare.h"

class DepthAggregater
{
public:

    DepthAggregater(QuoteSourceCallbackInterface* engine);
    ~DepthAggregater();

    void start();

    void set_engine(QuoteSourceCallbackInterface* ptr) { engine_ = ptr; }

    void on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote);

    void set_config(unordered_map<TSymbol, SMixerConfig>& new_config);

private:

    // 缓存数据
    void _calc_symbol(const TSymbol& symbol, const SMixerConfig& config, type_seqno seqno); 

    void _thread_loop();

private:
    // callback
    QuoteSourceCallbackInterface *                                  engine_{nullptr};

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
