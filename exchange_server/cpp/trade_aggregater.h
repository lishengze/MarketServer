#pragma once
#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"

#include "struct_define.h"


class TradeAggregater:public bcts::comm::QuoteSourceCallbackInterface
{
public:


    void set_comm(bcts::comm::Comm*  comm){ p_comm_ = comm;}

    void set_config(unordered_map<TSymbol, SMixerConfig>& new_config)
    {
        symbol_config_ = new_config;
    }

    virtual void on_trade( TradeData& trade);

private:
    // callback

    bcts::comm::Comm*                         p_comm_{nullptr};

    unordered_map<TSymbol, TradeData>         last_trade_map_;

    unordered_map<TSymbol, SMixerConfig>      symbol_config_;
};