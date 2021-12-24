
#include "depth_riskctrl.h"

#include "Log/log.h"
#include "util/tool.h"

/////////////////////////////////////////////////////////////////////
DepthRiskCtrl::DepthRiskCtrl():thread_run_(true)
{

}

DepthRiskCtrl::~DepthRiskCtrl()
{
    thread_run_ = false;
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}


void DepthRiskCtrl::start()
{

}

void DepthRiskCtrl::on_snap( SDepthQuote& quote)
{
    try
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };

        quotes_[quote.symbol][quote.exchange] = quote;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthRiskCtrl::on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthRiskCtrl::on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthRiskCtrl::on_hedge_config_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config)
{
    try
    {

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

// 账户相关回调
void DepthRiskCtrl::on_account_update(const AccountInfo& info)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

// 未对冲订单簿更新
void DepthRiskCtrl::on_order_update(const string& symbol, 
                        const SOrder& order, 
                        const vector<SOrderPriceLevel>& asks, 
                        const vector<SOrderPriceLevel>& bids)
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}                        
