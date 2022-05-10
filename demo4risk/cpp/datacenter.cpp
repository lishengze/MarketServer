#include "risk_controller_config.h"
#include "datacenter.h"
#include "grpc_server.h"
#include "converter.h"
#include "updater_configuration.h"

#include "updater_quote.h"
#include "Log/log.h"
#include "util/tool.h"


/////////////////////////////////////////////////////////////////////////////////
DataCenter::DataCenter() 
{
    if (FEE_RISKCTRL_OPEN)
    {
        LOG_INFO("Add fee_worker_");
        riskctrl_work_line_.add_worker(&fee_worker_);
    }

    if (BIAS_RISKCTRL_OPEN)
    {
        LOG_INFO("Add quotebias_worker_");
        riskctrl_work_line_.add_worker(&quotebias_worker_);
    }

    if (WATERMARK_RISKCTRL_OPEN)
    {
        LOG_INFO("Add watermark_worker_");
        riskctrl_work_line_.add_worker(&watermark_worker_);
    }

    if (ACCOUNT_RISKCTRL_OPEN)
    {
        LOG_INFO("Add account_worker_");
        riskctrl_work_line_.add_worker(&account_worker_);
    }

    if (ORDER_RISKCTRL_OPEN)
    {
        LOG_INFO("Add orderbook_worker_");
        riskctrl_work_line_.add_worker(&orderbook_worker_);
    }    

    if (PRICESION_RISKCTRL_OPEN)
    {
        LOG_INFO("Add pricesion_worker_");
        riskctrl_work_line_.add_worker(&pricesion_worker_);
    }

    start_check_symbol();

    if(PUBLISH_CTRL_OPEN)
    {
        start_publish();
    }    
}

DataCenter::~DataCenter() {
    if (check_thread_.joinable())
    {
        check_thread_.join();
    }

    if (publish_thread_.joinable())
    {
        publish_thread_.join();
    }
}

void DataCenter::start_publish()
{
    try
    {
        LOG_DEBUG("start_publish");

        publish_thread_ = std::thread(&DataCenter::publish_main, this);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DataCenter::update_publish_data_map()
{
    try
    {
        std::lock_guard<std::mutex> lk(params_.mutex_market_risk_config_);

        for (auto iter: params_.market_risk_config)
        {
            frequency_map_[iter.first] = iter.second.PublishFrequency;

            LOG_DEBUG("PublishFrequency: " + iter.first + ", " + std::to_string(iter.second.PublishFrequency));            
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DataCenter::publish_main()
{
    try
    {
        unsigned long long time = 0;
        while (true)
        {
            std::list<string> publish_list;

            int sleep_millsecs = get_sleep_millsecs(time, publish_list);

            // LOG_DEBUG("Time: "+std::to_string(time) 
            //          + ", sleep_millsecs: " + std::to_string(sleep_millsecs) + "; ");
            
            // for (auto symbol:publish_list)
            // {
            //     LOG_DEBUG("Pub: " + symbol + ", pbf: " + std::to_string(frequency_map_[symbol]));
            // }

            process_symbols(publish_list);
            
            time += sleep_millsecs;

            if (time > 24*60*60*1000) time = 0;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_millsecs));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

int DataCenter::get_sleep_millsecs(unsigned long long time, std::list<string>& publish_list)
{
    try
    {
        std::lock_guard<std::mutex> lk(mutex_frequency_map_);

        int sleep_millsecs = 0;


        if (frequency_map_.size() == 0) sleep_millsecs = 10*1000;
        else
        {
            int min_sleep_secs = frequency_map_.begin()->second;

            for (auto iter:frequency_map_)
            {
                int cur_sleep_millsecs = get_sleep_millsecs(time, iter.second);

                if (cur_sleep_millsecs == 0)
                {
                    publish_list.push_back(iter.first);
                }
                else if (!sleep_millsecs || cur_sleep_millsecs <= sleep_millsecs)
                {
                    sleep_millsecs = cur_sleep_millsecs;
                }

                if (min_sleep_secs > iter.second) min_sleep_secs = iter.second;
            }

            if (sleep_millsecs == 0)
            {
                sleep_millsecs = min_sleep_secs;
            }
        }

        return sleep_millsecs;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return 0;
}

int DataCenter::get_sleep_millsecs(unsigned long long cur_time, int fre)
{
    try
    {
        if (cur_time == 0) return fre;
        else if (cur_time % fre == 0) return 0;
        else return fre * (cur_time / fre + 1) - cur_time;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return -1;
}

void DataCenter::add_quote(SInnerQuote& quote)
{    
    // if (quote.symbol == CONFIG->test_symbol)
    // {
    //     LOG_DEBUG("\nOriginal Quote: " + " " + quote.symbol + " " + quote_str(quote, 3));
    // } 

    precheck_quote(quote);
    
    set_src_quote(quote);

    if (!PUBLISH_CTRL_OPEN)
    {
        process(quote);
    }        
};

void DataCenter::process_symbols(std::list<string> symbol_list)
{
    try
    {
        std::lock_guard<std::mutex> lk(mutex_original_datas_);

        for (auto symbol:symbol_list)
        {
            if (original_datas_.find(symbol) == original_datas_.end())
            {
                LOG_WARN("No Original Data For " + symbol);
                continue;
            }
            else
            {
                SInnerQuote& original_quote = original_datas_[symbol];

                process(original_quote);
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

bool DataCenter::process(const SInnerQuote& src_quote)
{
    try
    {
        SInnerQuote dst_quote;

        PipelineContent context(params_);

        if (src_quote.symbol == CONFIG->test_symbol)
        {
            LOG_DEBUG("------------------ RiskCtrl Begin ------------------");
        }
        riskctrl_work_line_.run(src_quote, context, dst_quote);

        filter_zero_volume(dst_quote);

        if (!check_quote_time(src_quote, dst_quote)) return false;
        
        if (check_abnormal_quote(dst_quote))
        {
            LOG_WARN("\n" + dst_quote.symbol + " _publish_quote \n" + quote_str(dst_quote));
        }  

        if (!check_quote_publish(dst_quote))
        {
            LOG_WARN(dst_quote.symbol + " not published!" );
            return false;
        }

        _publish_quote(dst_quote);

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return false;
}

void DataCenter::set_src_quote(const SInnerQuote& quote)
{
    try
    {
        std::lock_guard<std::mutex> lk(mutex_original_datas_);
        original_datas_[quote.symbol] = quote;    
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void DataCenter::precheck_quote(const SInnerQuote& quote)
{
    try
    {
        set_check_symbol_map(quote.symbol);

        if (check_abnormal_quote( const_cast<SInnerQuote&>(quote)))
        {
            LOG_WARN("\n" + quote.symbol + " raw quote\n" + quote_str(quote));
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}


bool DataCenter::check_quote_time(const SInnerQuote& src_quote, const SInnerQuote& processed_quote)
{
    try
    {
        // 检查是否发生变化
        std::lock_guard<std::mutex> lk(mutex_riskctrl_datas_);
        auto iter = riskctrl_datas_.find(src_quote.symbol);
        if( iter != riskctrl_datas_.end() ) 
        {
            const SInnerQuote& last_quote = iter->second;
            if( src_quote.time_origin < last_quote.time_origin ) 
            {
                LOG_WARN("Quote Error last_quote.time: " + std::to_string(last_quote.time_origin) + ", processed quote_time: " + std::to_string(src_quote.time_origin));
                return false;
            }
        }        
        riskctrl_datas_[src_quote.symbol] = processed_quote;
        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;
}

void DataCenter::change_account(const AccountInfo& info)
{   
    {
        std::lock_guard<std::mutex> inner_lock{ params_.mutex_account_config_ };
        params_.account_config = info;
    }

    // _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, MarketRiskConfig>& config)
{   
    {
        std::lock_guard<std::mutex> inner_lock{ params_.mutex_market_risk_config_ };
        params_.market_risk_config = config;

        LOG_INFO("DataCenter::change MarketRiskConfig");
        for (auto iter:params_.market_risk_config)
        {
            LOG_INFO("\n" + iter.first + "\n" + iter.second.desc());
        }    
    }

    update_publish_data_map();

    // _push_to_clients();
}

void DataCenter::change_configuration(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        std::lock_guard<std::mutex> inner_lock{params_.mutex_symbol_config_};
        params_.symbol_config = config;

        LOG_INFO("DataCenter::change SymbolConfiguration");
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

void DataCenter::change_configuration(const map<TSymbol, map<TExchange, HedgeConfig>>& config)
{
    try
    {
        std::lock_guard<std::mutex> inner_lock{ mutex_config_};
        params_.hedge_config = config;

        LOG_INFO("DataCenter::change HedgeConfig");
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

void DataCenter::change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{

    {
        std::lock_guard<std::mutex> inner_lock{ params_.mutex_cache_order_ };
        params_.cache_order[symbol] = make_pair(asks, bids);
    }


    _push_to_clients(symbol);
}

void DataCenter::_push_to_clients(const TSymbol& symbol) 
{
    try
    {
        std::lock_guard<std::mutex> lk(mutex_original_datas_);

        if( symbol == "" ) 
        {
            for( auto iter = original_datas_.begin() ; iter != original_datas_.end() ; ++iter ) 
            {
                process(iter->second);
            }
        } 
        else 
        {
            auto iter = original_datas_.find(symbol);
            if(iter == original_datas_.end())
            {
                LOG_ERROR("No Original Data For " + symbol);
                return;
            }

            process(iter->second);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DataCenter::_publish_quote(const SInnerQuote& quote) 
{      
    try
    {
        std::shared_ptr<MarketStreamData> ptrData1(new MarketStreamData);
        innerquote_to_msd2(quote, ptrData1.get(), false);
        LOG->record_output_info("Hedge_" + quote.symbol + "_" + quote.exchange);
        for( const auto& v : callbacks_) 
        {
            v->publish4Hedge(quote.symbol, ptrData1, NULL);
        }    

        std::shared_ptr<MarketStreamData> ptrData2(new MarketStreamData);
        innerquote_to_msd2(quote, ptrData2.get(), true);    
        LOG->record_output_info("Broker_" + quote.symbol + "_" + quote.exchange);
        for( const auto& v : callbacks_) 
        {
            v->publish4Broker(quote.symbol, ptrData2, NULL);
        }

        // send to clients
        std::shared_ptr<MarketStreamDataWithDecimal> ptrData3(new MarketStreamDataWithDecimal);
        innerquote_to_msd3(quote, ptrData3.get(), true);   
        LOG->record_output_info("Client_" + quote.symbol + "_" + quote.exchange);
        for( const auto& v : callbacks_) 
        {
            v->publish4Client(quote.symbol, ptrData3, NULL);
        }    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

bool DataCenter::check_quote_publish(SInnerQuote& quote)
{
    bool result = false;    

    try
    {
        std::lock_guard<std::mutex> lk(params_.mutex_market_risk_config_);
        if (params_.market_risk_config.find(quote.symbol) != params_.market_risk_config.end())
        {
            result = params_.market_risk_config[quote.symbol].IsPublish;

            if (!result) return result;

            uint32 publis_level = params_.market_risk_config[quote.symbol].PublishLevel;
            map<SDecimal, SInnerDepth> new_asks;
            map<SDecimal, SInnerDepth> new_bids;     

            uint32 i = 0;
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
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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

QuoteResponse_Result _calc_otc_by_volume(const map<SDecimal, SInnerDepth>& depths, bool is_ask, MarketRiskConfig& config, double volume, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0; 
    SDecimal total_amount = 0;

    if( is_ask ) 
    {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            
            double price = iter->first.get_value();
            double old_price = price;

            if (BIAS_RISKCTRL_OPEN)
            {
                reset_price(price, config, true);
            }
            

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                total_amount += iter->second.total_volume * price;

                LOG_DEBUG("cur_p: " + std::to_string(price)
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", ask;");

            } else {
                total_amount += (volume - total_volume.get_value()) * price;
                total_volume = volume;

                LOG_DEBUG("done cur_p: " +  std::to_string(price) 
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", ask;");
                break;
            }
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            double old_price = price;
            reset_price(price, config, false);

            if( (total_volume + iter->second.total_volume) <= volume ) {
                total_volume += iter->second.total_volume;
                double trade_amout = iter->second.total_volume.get_value() * price;
                total_amount += trade_amout;

                LOG_DEBUG("cur_p: " + std::to_string(price)
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", total_v: " + total_volume.get_str_value()
                        + ", cur_a: " + std::to_string(trade_amout)
                        + ", total_a: " + total_amount.get_str_value()
                        + ", otc_v: " + std::to_string(volume)                        
                        + ", bid;");

            } else {
                double trade_volume = volume - total_volume.get_value();
                double trade_amout = trade_volume * price;
                total_amount += trade_amout;
                total_volume += trade_volume;

                LOG_DEBUG("[done] cur_price: " + std::to_string(price) 
                        + ", risk_p: " + std::to_string(old_price)
                        + ", cur_v: " + iter->second.total_volume.get_str_value() 
                        + ", trade_v: " + std::to_string(trade_volume)
                        + ", total_v: " + total_volume.get_str_value()
                        + ", cur_a: " + std::to_string(trade_amout)
                        + ", total_a: " + total_amount.get_str_value()
                        + ", otc_v: " + std::to_string(volume)
                        + ", bid;");
                break;
            }
        }
    }

    if( total_volume < volume )
    {
        string msg = "_calc_otc_by_volume failed depth_sum_volume is: " 
                + total_volume.get_str_value() 
                + ", request_volume: " + std::to_string(volume);
        LOG_WARN(msg);
        LOG_DEBUG(msg);
        
        return QuoteResponse_Result_NOT_ENOUGH_VOLUME;
    }
        
    price = total_amount.get_value() / total_volume.get_value();
    double ori_price = price.get_value();

    if (config.OTCOffsetKind == 1)
    {
        if( is_ask ) {
            price *= ( 1 + config.OtcOffset); 
        } else {

            if (config.OtcOffset > 1) config.OtcOffset = 1;
            price *= ( 1 - config.OtcOffset); 
        }
    }
    else if (config.OTCOffsetKind == 2)
    {
        if( is_ask ) {
            if (price < config.OtcOffset) price = 0;
            else price -= config.OtcOffset;
        } else {
            price += config.OtcOffset;
        }        
    }

    LOG_DEBUG("ori_price: " + std::to_string(ori_price) 
            + ", bias_price: " + price.get_str_value() 
            + ", bias_kind: " + std::to_string(config.OTCOffsetKind)
            + ", bias_value: " + std::to_string(config.OtcOffset)
            + ", precise: " + std::to_string(precise));

    price.scale(precise, is_ask);

    LOG_DEBUG("Scaled Price: " + price.get_str_value());

    return QuoteResponse_Result_OK;
}

QuoteResponse_Result _calc_otc_by_amount(const map<SDecimal, SInnerDepth>& depths, bool is_ask, MarketRiskConfig& config, double otc_amount, SDecimal& price, uint32 precise)
{
    SDecimal total_volume = 0;
    SDecimal total_amount = 0;
    if( is_ask ) 
    {
        for( auto iter = depths.begin() ; iter != depths.end() ; iter ++ ) {
            double price = iter->first.get_value();

            if (BIAS_RISKCTRL_OPEN)
            {
                reset_price(price, config, true);
            }

            SDecimal cur_amounts = iter->second.total_volume * price;
            if( (total_amount + cur_amounts) <= otc_amount ) {
                total_volume += iter->second.total_volume;
                total_amount += cur_amounts;

                LOG_DEBUG("cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", ask;");
            } else {
                total_volume += (otc_amount - total_amount.get_value()) / price;
                total_amount = otc_amount;

                LOG_DEBUG("done cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", ask;");
                break;
            }        
        }
    } 
    else 
    {
        for( auto iter = depths.rbegin() ; iter != depths.rend() ; iter ++ ) {
            double price = iter->first.get_value();
            reset_price(price, config, false);
            SDecimal cur_amounts = iter->second.total_volume * price;
            if( (total_amount + cur_amounts) <= otc_amount ) {
                total_volume += iter->second.total_volume;
                total_amount += cur_amounts;

                LOG_DEBUG("cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", bid;");                
            } else {
                total_volume += (otc_amount - total_amount.get_value()) / price;
                total_amount = otc_amount;
                LOG_DEBUG("done cur_price: " + iter->first.get_str_value() 
                        + ", cur_volume: " + iter->second.total_volume.get_str_value() 
                        + ", cur_amount: " + cur_amounts.get_str_value()
                        + ", total_amount: " + total_amount.get_str_value()
                        + ", otc_amount: " + std::to_string(otc_amount)
                        + ", bid;");
                break;
            }
        }
    }
    if( total_amount < otc_amount )
    {
        string msg = "_calc_otc_by_amount failed depth_sum_volume is: " 
                + total_amount.get_str_value() 
                + ", otc_amount: " + std::to_string(otc_amount);
        LOG_WARN(msg);
        LOG_DEBUG(msg);
        return QuoteResponse_Result_NOT_ENOUGH_AMOUNT;
    }
        

    price = total_amount.get_value() / total_volume.get_value();
    double ori_price = price.get_value();

    if (config.OTCOffsetKind == 1)
    {
        if( is_ask ) {
            price *= ( 1 + config.OtcOffset); 
        } else {

            if (config.OtcOffset > 1) config.OtcOffset = 1;
            price *= ( 1 - config.OtcOffset); 
        }
    }
    else if (config.OTCOffsetKind == 2)
    {
        if( is_ask ) {
            if (price < config.OtcOffset) price = 0;
            else price -= config.OtcOffset;
        } else {
            price += config.OtcOffset;
        }        
    }

    LOG_DEBUG("ori_price: " + std::to_string(ori_price) 
            + ", bias_price: " + price.get_str_value() 
            + ", bias_kind: " + std::to_string(config.OTCOffsetKind)
            + ", bias_value: " + std::to_string(config.OtcOffset)
            + ", precise: " + std::to_string(precise));

    price.scale(precise, is_ask);

    LOG_DEBUG("Scaled Price: " + price.get_str_value());
    
    return QuoteResponse_Result_OK;
}

bool DataCenter::get_snaps(vector<SInnerQuote>& snaps)
{
    std::lock_guard<std::mutex> inner_lock{ mutex_riskctrl_datas_ };
    for( const auto& v: riskctrl_datas_ ) {
        snaps.push_back(v.second);
    }
    return true;
}

QuoteResponse_Result DataCenter::otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double volume, double amount, SDecimal& dst_price)
{

    string direction_str = direction==QuoteRequest_Direction_BUY?"buy":"sell";
    LOG_DEBUG("[otc_query] " + exchange + "." + symbol 
            + ", direction=" + direction_str
            + ", volume=" + std::to_string(volume)
            + ", amount=" + std::to_string(amount));

    SInnerQuote* quote = nullptr;

    {
        std::lock_guard<std::mutex> inner_lock{ mutex_riskctrl_datas_ };
        auto iter = riskctrl_datas_.find(symbol);

        if(iter == riskctrl_datas_.end())
        {
            LOG_WARN("OTC Request Symbol " + symbol + " has no data");
            return QuoteResponse_Result_WRONG_SYMBOL;
        }
        quote = &(iter->second);
    }

    LOG_DEBUG("\n OTC Quote Data \n" + quote_str(*quote, 5));

    uint32 precise = 8;

    {
        std::lock_guard<std::mutex> lk(params_.mutex_symbol_config_);
        if (params_.symbol_config.find(symbol) != params_.symbol_config.end())
        {
            precise = params_.symbol_config[symbol].PricePrecision;
        }
        else
        {            
            LOG_WARN("symbol_config has no config for " + symbol);
            return QuoteResponse_Result_WRONG_SYMBOL;
        }

        if (params_.market_risk_config.find(symbol) == params_.market_risk_config.end())
        {           
            LOG_WARN("market_risk_config has no config for " + symbol);
            return QuoteResponse_Result_WRONG_SYMBOL;
        }
    }


    if( volume > 0 )
    {        
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_volume(quote->asks, true, params_.market_risk_config[symbol], volume, dst_price, precise);
        } else {
            return _calc_otc_by_volume(quote->bids, false, params_.market_risk_config[symbol], volume, dst_price, precise);   
        }
    } 
    else
    {        
        if( direction == QuoteRequest_Direction_BUY ) {
            return _calc_otc_by_amount(quote->asks, true, params_.market_risk_config[symbol], amount, dst_price, precise);
        } else { 
            return _calc_otc_by_amount(quote->bids, false, params_.market_risk_config[symbol], amount, dst_price, precise);
        }
    }

    return QuoteResponse_Result_WRONG_DIRECTION;
}

void DataCenter::get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations)
{
    watermark_worker_.query(watermarks);

    {
        std::lock_guard<std::mutex> inner_lock{ params_.mutex_account_config_ };
        for( const auto&v : params_.account_config.hedge_accounts_ ) {
            const TExchange& exchange = v.first;
            for( const auto& v2 : v.second.currencies ) {
                const TSymbol& symbol = v2.first;
                accounts[exchange][symbol] = v2.second.amount;
            }
        }
    }

    {
        std::lock_guard<std::mutex> inner_lock{ params_.mutex_market_risk_config_ };
        for( const auto&v : params_.market_risk_config ) {
            const TSymbol& symbol = v.first;
            configurations[symbol] = v.second.desc();
        }
    }

}

void DataCenter::hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade)
{
    try
    {
        std::lock_guard<std::mutex> lk(params_.mutex_hedage_order_info_);
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
       LOG_ERROR(e.what());
    }
}

void DataCenter::start_check_symbol()
{
    try
    {
        check_thread_ = std::thread(&DataCenter::check_symbol_main, this);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DataCenter::set_check_symbol_map(TSymbol symbol)
{
    try
    {
        std::lock_guard<std::mutex> lk(check_symbol_mutex_);

        check_symbol_map_[symbol] = 1;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DataCenter::check_symbol_main()
{
    try
    {
        while(true)
        {
            check_symbol();

            std::this_thread::sleep_for(std::chrono::seconds(CONFIG->check_symbol_secs));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void DataCenter::check_symbol()
{
    try
    {
        std::lock_guard<std::mutex> lk(check_symbol_mutex_);

        for (auto& iter:check_symbol_map_)
        {
            const TSymbol& symbol = iter.first;

            LOG_DEBUG(symbol + ": " + std::to_string(iter.second));

            if (iter.second == 0)
            {
                LOG_WARN(symbol + " is dead");

                erase_outdate_symbol(symbol);

                iter.second = -1;
            }
            else if (iter.second == 1)
            {                
                iter.second = 0;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

}

void DataCenter::erase_outdate_symbol(TSymbol symbol)
{
    try
    {
        {
            std::lock_guard<std::mutex> lk(mutex_original_datas_);

            if (original_datas_.find(symbol)!= original_datas_.end())
            {
                original_datas_.erase(symbol);
            }
        }

        {
            std::lock_guard<std::mutex> lk(mutex_riskctrl_datas_);
            if (riskctrl_datas_.find(symbol) != riskctrl_datas_.end())
            {
                riskctrl_datas_.erase(symbol);
            }    
        }
    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}
