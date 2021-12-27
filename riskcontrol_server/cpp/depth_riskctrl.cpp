
#include "depth_riskctrl.h"

#include "Log/log.h"
#include "util/tool.h"
#include "native_config.h"

void DepthRiskCtrl::start()
{

}

                     
DepthRiskCtrl::DepthRiskCtrl() 
{
    if (ACCOUNT_RISKCTRL_OPEN)
    {
        pipeline_.add_worker(&account_worker_);
    }

    if (ORDER_RISKCTRL_OPEN)
    {
        pipeline_.add_worker(&orderbook_worker_);
    }

    pipeline_.add_worker(&quotebias_worker_);
    pipeline_.add_worker(&watermark_worker_);
    pipeline_.add_worker(&pricesion_worker_);
}

DepthRiskCtrl::~DepthRiskCtrl() {

}

void DepthRiskCtrl::on_snap(SDepthQuote& quote)
{    
    // if (quote.symbol == "BTC_USDT")
    // {
    //     LOG_INFO(quote.str());
    // } 

    // check_exchange_volume(quote);

    if (filter_zero_volume(const_cast<SDepthQuote&>(quote), filter_quote_mutex_))
    {
        LOG_WARN("\n" + quote.symbol + " raw quote\n" + quote_str(quote));
        if (quote.asks.size() == 0 && quote.bids.size() == 0)
        {
            return;
        }
    }    

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    
    // 存储原始行情
    datas_[quote.symbol] = quote;    
    // 发布行情
    _publish_quote(quote);    

};

void DepthRiskCtrl::on_account_update(const AccountInfo& info)
{   
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.account_config = info;
    _push_to_clients();
}

void DepthRiskCtrl::on_order_update(const string& symbol, const SOrder& order, 
                                    const vector<SOrderPriceLevel>& asks, 
                                    const vector<SOrderPriceLevel>& bids)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.cache_order[symbol] = make_pair(asks, bids);

    _push_to_clients(symbol);
}


void DepthRiskCtrl::on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config)
{   
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    params_.quote_config = config;

    LOG_TRACE("DepthRiskCtrl::change MarketRiskConfig");
    for (auto iter:params_.quote_config)
    {
        LOG_TRACE("\n" + iter.first + "\n" + iter.second.desc());
    }    
    _push_to_clients();
}

void DepthRiskCtrl::on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.symbol_config = config;

        LOG_INFO("DepthRiskCtrl::change SymbolConfiguration");
        for (auto iter:params_.symbol_config)
        {
            LOG_INFO("\n" + iter.first + " " + iter.second.desc());
        }
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
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.hedge_config = config;

        LOG_INFO("DepthRiskCtrl::change HedgeConfig");
        for (auto iter1:params_.hedge_config)
        {
            for (auto iter2:iter1.second)
            {
                LOG_INFO(iter2.second.str());
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthRiskCtrl::_push_to_clients(const TSymbol& symbol) 
{
    if( symbol == "" ) 
    {
        for( auto iter = datas_.begin() ; iter != datas_.end() ; ++iter ) 
        {
            _publish_quote(iter->second);
        }
    } else {
        auto iter = datas_.find(symbol);
        if( iter == datas_.end() )
            return;

        _publish_quote(iter->second);
    }
}

void DepthRiskCtrl::_publish_quote(const SDepthQuote& quote) 
{    
    SDepthQuote newQuote;
    newQuote.origin_time = utrade::pandora::NanoTime();
    pipeline_.run(quote, params_, newQuote);

    // 检查是否发生变化
    auto iter = last_datas_.find(quote.symbol);
    if( iter != last_datas_.end() ) {
        const SDepthQuote& last_quote = iter->second;
        if( quote.sequence_no < last_quote.sequence_no ) {
            LOG_WARN("Quote Error last_quote.sequence_no: " + std::to_string(last_quote.sequence_no) 
                    + ", processed sequence_no: " + std::to_string(quote.sequence_no));
            return;
        }
    }

    last_datas_[quote.symbol] = newQuote;

    if (!check_quote(newQuote))
    {
        LOG_WARN(quote.symbol + " not published!" );
        return;
    }

    if (filter_zero_volume(newQuote, filter_quote_mutex_))
    {
        LOG_WARN("\n" + newQuote.symbol + " _publish_quote \n" + quote_str(newQuote));
        if (newQuote.asks.size() == 0 && newQuote.bids.size() == 0)
        {
            return;
        } 
    }     

    newQuote.exchange = RISKCTRL_MIX_EXCHANGE_NAME;   

    p_comm_->publish_depth(newQuote);
}

bool DepthRiskCtrl::check_quote(SDepthQuote& quote)
{
    bool result = false;
    if (params_.quote_config.find(quote.symbol) != params_.quote_config.end())
    {
        result = params_.quote_config[quote.symbol].IsPublish;

        if (!result) return result;

        uint32 publis_level = params_.quote_config[quote.symbol].PublishLevel;
        map<SDecimal, SDepth> new_asks;
        map<SDecimal, SDepth> new_bids;     

        int i = 0;
        for (auto iter = quote.asks.begin(); iter != quote.asks.end() && i < publis_level; ++iter, ++i)
        {
            new_asks[iter->first] = iter->second;
        }   
        quote.asks.swap(new_asks);

        i = 0;
        for(auto iter = quote.bids.rbegin(); iter != quote.bids.rend() && i < publis_level; ++iter, ++i)
        {
            new_bids[iter->first] = iter->second;
        }
        quote.bids.swap(new_bids);
    }

    return result;
    
}

void reset_price(double& price, MarketRiskConfig& config, bool is_ask)
{
    try
    {
        double price_bias = config.PriceOffset;

        if (config.PriceOffsetKind == 1)
        {
            if( is_ask ) 
            {
                price /= ( 1 + price_bias );
            } 
            else 
            {
                if( price_bias < 1 )
                    price /= ( 1 - price_bias);
                else
                    price = 0;
            }
        }
        else if (config.PriceOffsetKind == 2)
        {
            if( is_ask ) 
            {
                price -= price_bias;
            } 
            else 
            {
                price += price_bias;
            }

            price = price > 0 ? price : 0;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
}

bool DepthRiskCtrl::get_snaps(vector<SDepthQuote>& snaps)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( const auto& v: last_datas_ ) {
        snaps.push_back(v.second);
    }
    return true;
}

void DepthRiskCtrl::get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations)
{
    watermark_worker_.query(watermarks);

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( const auto&v : params_.account_config.hedge_accounts_ ) {
        const TExchange& exchange = v.first;
        for( const auto& v2 : v.second.currencies ) {
            const TSymbol& symbol = v2.first;
            accounts[exchange][symbol] = v2.second.amount;
        }
    }
    for( const auto&v : params_.quote_config ) {
        const TSymbol& symbol = v.first;
        configurations[symbol] = v.second.desc();
    }
}

void DepthRiskCtrl::hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade)
{
    try
    {
        if (params_.hedage_order_info.find(symbol) == params_.hedage_order_info.end() && !is_trade)
        {
            params_.hedage_order_info[symbol] = HedgeInfo(symbol, price, amount, direction, is_trade);
        }
        else
        {
            params_.hedage_order_info[symbol].set(symbol, price, amount, direction, is_trade);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":"  << __FUNCTION__ <<"."<< __LINE__ << " " <<  e.what() << '\n';
    }
}
