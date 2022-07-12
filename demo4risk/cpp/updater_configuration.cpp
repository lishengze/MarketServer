#include "updater_configuration.h"
#include "risk_controller_config.h"
#include "pandora/util/float_util.h"

void ConfigurationClient::_run()
{
    add_listener("BCTS", "MarketRisk", risk_watcher_);
    add_listener("BCTS", "SymbolParams", symbol_watcher_);
    add_listener("BCTS", "HedgeParams", hedger_watcher_);

    while( is_running() ) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ConfigurationClient::config_changed(const string& group, const string& dataid, const NacosString &configInfo)
{

    if( group == "BCTS" ) {

        if (dataid == "MarketRisk")
        {
            // risk_params_ = configInfo;

            // if( risk_params_.length() > 0 ) 
            // {
            //     this->_parse_config();
            // }

            load_market_risk(configInfo);
        }

        if( dataid == "HedgeParams" ) 
        {
            // hedge_params_ = configInfo;

            load_hedge_params(configInfo);          
        }        

        if (dataid == "SymbolParams")
        {
            load_symbol_params(configInfo);
        }
        
    } 
    else 
    {
        LOG_ERROR("\n---Unknown Config: "+ group + "."+ dataid + ": \n" + configInfo.c_str());
        return;
    }
    

}

void ConfigurationClient::load_market_risk(const NacosString &configInfo)
{
    try
    {
        Document risks;
        risks.Parse(configInfo.c_str());
        LOG_INFO("\nMarketRisk OriInfo: \n" + configInfo.c_str());

        if(risks.HasParseError())
        {
            LOG_ERROR("parse MarketRisk error " + std::to_string(risks.GetParseError()));
            return;
        }
        if( !risks.IsArray() )
        {
            LOG_ERROR("MarketRisk is not array!");
            return;   
        }
                 
        map<TSymbol, MarketRiskConfig> output;
        for( auto iter = risks.Begin() ; iter != risks.End() ; iter++ ) 
        {
            string symbol = helper_get_string(*iter, "symbol_id", "");
            bool IsPublish = helper_get_bool(*iter, "switch", true);
            uint32 PublishFrequency = helper_get_uint32(*iter, "publish_frequency", 1); // 暂时没用
            uint32 PublishLevel = helper_get_uint32(*iter, "publish_level", 1); // 暂时没用

            uint32 PriceOffsetKind = helper_get_uint32(*iter, "price_offset_kind", 1); // 暂时没用
            double PriceOffset = helper_get_double(*iter, "price_offset", 0);

            uint32 AmountOffsetKind = helper_get_uint32(*iter, "amount_offset_kind", 1); // 暂时没用
            double AmountOffset = helper_get_double(*iter, "amount_offset", 0);

            double DepositFundRatio = helper_get_double(*iter, "deposit_fund_ratio", 100); // 暂时没用

            uint32 OTCOffsetKind = helper_get_uint32(*iter, "quoted_offset_kind", 1); // 暂时没用
            double OtcOffset = helper_get_double(*iter, "quoted_offset", 0);

            double OTCAmountSize = helper_get_double(*iter, "quoted_amount_size", 0);
            double OTCAmountOffset = helper_get_double(*iter, "quoted_amount_offset", 0);
            
            if (PriceOffsetKind != 1 && PriceOffsetKind != 2) 
            {
                LOG_WARN(symbol + " PriceOffsetKind should be 1 or 2 ,now is: " + std::to_string(PriceOffsetKind));
                continue;
            }
            if (PriceOffset < 0 || PriceOffset >= 1)
            {
                LOG_WARN(symbol + " PriceOffset should in (0, 1),now is: " + std::to_string(PriceOffset));
                continue;
            }

            if (AmountOffsetKind != 1 && AmountOffsetKind != 2) 
            {
                LOG_WARN(symbol + " AmountOffsetKind should be 1 or 2 ,now is: " + std::to_string(AmountOffsetKind));
                continue;
            }

            if (AmountOffset < 0 || AmountOffset >= 1)
            {
                LOG_WARN(symbol + " AmountOffset should in (0, 1),now is: " + std::to_string(AmountOffset));
                continue;
            }            

            if (OTCOffsetKind != 1 && OTCOffsetKind != 2) 
            {
                LOG_WARN(symbol + " OTCOffsetKind should be 1 or 2 ,now is: " + std::to_string(OTCOffsetKind));
                continue;
            }

            if (OtcOffset < 0 || OtcOffset >= 1) 
            {
                LOG_WARN(symbol + " OtcOffset should in (0, 1) ,now is: " + std::to_string(PriceOffsetKind));
                continue;
            }                      

            if (utrade::pandora::less_equal(OTCAmountSize, 0) ) {
                LOG_WARN(symbol + " OTCAmountSize is " + std::to_string(OTCAmountSize));
                continue;                
            }

            if (utrade::pandora::less_equal(OTCAmountOffset, 0) ) {
                LOG_WARN(symbol + " OTCAmountOffset is " + std::to_string(OTCAmountOffset));
                continue;                
            }            

            if( symbol == "")
            {
                LOG_WARN("symbol is empty");
                continue;
            }            

            MarketRiskConfig cfg;

            cfg.symbol = symbol;
            cfg.PublishFrequency = PublishFrequency;
            cfg.PublishLevel = PublishLevel;
            cfg.PriceOffsetKind = PriceOffsetKind;
            cfg.PriceOffset = PriceOffset;
            cfg.AmountOffsetKind = AmountOffsetKind;
            cfg.AmountOffset = AmountOffset;
            cfg.DepositFundRatio = DepositFundRatio;

            cfg.OTCOffsetKind = OTCOffsetKind;
            cfg.OtcOffset = OtcOffset;

            cfg.OTCAmountOffset = OTCAmountOffset;
            cfg.OTCAmountSize = OTCAmountSize;

            cfg.IsPublish = IsPublish;

            output[symbol] = cfg;

            LOG_INFO("\nMarketRisk: " + symbol + "\n" + cfg.desc());
        }

        callback_->on_configuration_update(output);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void ConfigurationClient::load_symbol_params(const NacosString &configInfo)
{
    try
    {
        Document paramsObject;
        paramsObject.Parse(configInfo.c_str());

        LOG_INFO("\n--------------- load_symbol_params: \n" + configInfo.c_str());
    

        if(paramsObject.HasParseError())
        {
            LOG_WARN("ConfigurationClient::load_symbol_params error " + std::to_string(paramsObject.GetParseError()));
            return;
        }

        if( !paramsObject.IsArray() )
        {
            LOG_ERROR("paramsObject is not array");
            return ;
        }
            
        map<TSymbol, SymbolConfiguration> output;


        for( auto iter = paramsObject.Begin() ; iter != paramsObject.End() ; iter++ ) 
        {
            SymbolConfiguration config;
            config.SymbolId = helper_get_string(*iter, "symbol_id", "");
            config.SymbolKind = helper_get_string(*iter, "symbol_kind", "");
            config.Bid = helper_get_string(*iter, "bid", "");
            config.PrimaryCurrency = helper_get_string(*iter, "primary_currency", "");
            config.BidCurrency = helper_get_string(*iter, "bid_currency", "");
            config.SettleCurrency = helper_get_string(*iter, "settle_currency", "");

            config.Switch = helper_get_bool(*iter, "switch", false);

            config.AmountPrecision = helper_get_uint32(*iter, "amount_precision", 0);
            config.PricePrecision = helper_get_uint32(*iter, "price_precision", 0);
            config.SumPrecision = helper_get_uint32(*iter, "sum_precision", 0);

            config.MinUnit = helper_get_double(*iter, "min_unit", 0);
            config.MinChangePrice = helper_get_double(*iter, "min_change_price", 0);
            config.Spread = helper_get_double(*iter, "spread", 0);

            config.FeeKind = helper_get_uint32(*iter, "fee_kind", 0);

            config.TakerFee = helper_get_double(*iter, "taker_fee", 0);
            config.MakerFee = helper_get_double(*iter, "maker_fee", 0);
            config.MinOrder = helper_get_double(*iter, "min_order", 0);
            config.MaxOrder = helper_get_double(*iter, "max_order", 0);
            config.MinMoney = helper_get_double(*iter, "min_money", 0);
            config.MaxMoney = helper_get_double(*iter, "max_money", 0);       

            config.BuyPriceLimit = helper_get_double(*iter, "buy_price_limit", 0);         
            config.SellPriceLimit = helper_get_double(*iter, "sell_price_limit", 0);     

            config.MaxMatchLevel = helper_get_uint32(*iter, "max_match_level", 0);       

            config.OtcMinOrder = helper_get_double(*iter, "otc_min_order", 0);
            config.OtcMaxOrder = helper_get_double(*iter, "otc_max_order", 0);
            config.OtcMinPrice = helper_get_double(*iter, "otc_min_price", 0);
            config.OtcMaxPrice = helper_get_double(*iter, "otc_max_price", 0);                        


            config.User = helper_get_string(*iter, "user", "");
            config.Time = helper_get_string(*iter, "time", "");

            // cout << config.desc() << endl;

            output[config.SymbolId] = config;
        }

        callback_->on_configuration_update(output);

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

/*
	{
        "platform_id":"B2C2",
        "instrument":"BTC_USD",
        "switch":true,
        "buy_fund_ratio":0.3,
        "sell_fund_ratio":0.3,
        "price_precision":4,
        "amount_precision":4,
        "sum_precision":2,
        "min_unit":0.0001,
        "min_change_price":0.01,
        "max_margin":1,
        "max_order":100,
        "buy_price_limit":10000,
        "sell_price_limit":5000,
        "max_match_level":5,
        "fee_kind":1,
        "taker_fee":0,
        "maker_fee":0
    }
*/
void ConfigurationClient::load_hedge_params(const NacosString &configInfo)
{
    try
    {
        Document hedgeParamsObject;    
        hedgeParamsObject.Parse(configInfo.c_str());
        if(hedgeParamsObject.HasParseError())
        {
            LOG_WARN("parse HedgeParams error: " + std::to_string(hedgeParamsObject.GetParseError()));
            return;
        }
        LOG_INFO("\nHedgeRisk OriInfo: \n" + configInfo.c_str());

        map<TSymbol, map<TExchange, HedgeConfig>> output;

        for( auto iter = hedgeParamsObject.Begin() ; iter != hedgeParamsObject.End() ; iter++ ) 
        {
            HedgeConfig config;

            string symbol = helper_get_string(*iter, "instrument", "");
            string exchange = helper_get_string(*iter, "platform_id", "");

            double BuyFundPercent = helper_get_double(*iter, "buy_fund_ratio", 1);
            double SellFundPercent = helper_get_double(*iter, "sell_fund_ratio", 1);

            int FeeKind = helper_get_uint32(*iter, "fee_kind", 1);
            double TakerFee = helper_get_double(*iter, "taker_fee", 1);
            double MakerFee = helper_get_double(*iter, "maker_fee", 1);

            if (symbol == "")
            {
                LOG_WARN("Empty Symbol");
                continue;
            }            

            if (exchange == "")
            {
                LOG_WARN("Empty Exchange");
                continue;
            }

            if (FeeKind != 1 && FeeKind != 2)
            {
                LOG_WARN("Unknown FeeKind: " + std::to_string(FeeKind));
                continue;
            }

            config.exchange = exchange;
            config.symbol = symbol;
            config.BuyFundPercent = BuyFundPercent;
            config.SellFundPercent = SellFundPercent;
            config.FeeKind = FeeKind;
            config.TakerFee = TakerFee;
            config.MakerFee = MakerFee;

            output[symbol][exchange] = config;

            LOG_INFO(config.str());
        }        

        callback_->on_configuration_update(output);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool combine_config(const Document& risks, map<TSymbol, MarketRiskConfig>& output)
{    
    try
    {
        LOG_INFO("combine_config");

        if( !risks.IsArray() )
        {
            LOG_ERROR("risk is not array!");
            return false;   
        }
                 

        for( auto iter = risks.Begin() ; iter != risks.End() ; iter++ ) 
        {
            string symbol = helper_get_string(*iter, "symbol_id", "");
            bool IsPublish = helper_get_bool(*iter, "switch", true);
            uint32 PublishFrequency = helper_get_uint32(*iter, "publish_frequency", 1); // 暂时没用
            uint32 PublishLevel = helper_get_uint32(*iter, "publish_level", 1); // 暂时没用

            uint32 PriceOffsetKind = helper_get_uint32(*iter, "price_offset_kind", 1); // 暂时没用
            double PriceOffset = helper_get_double(*iter, "price_offset", 0);

            uint32 AmountOffsetKind = helper_get_uint32(*iter, "amount_offset_kind", 1); // 暂时没用
            double AmountOffset = helper_get_double(*iter, "amount_offset", 0);

            double DepositFundRatio = helper_get_double(*iter, "deposit_fund_ratio", 100); // 暂时没用

            uint32 OTCOffsetKind = helper_get_uint32(*iter, "poll_offset_kind", 1); // 暂时没用
            double OtcOffset = helper_get_double(*iter, "poll_offset", 0);
            
            if (PriceOffsetKind != 1 && PriceOffsetKind != 2) 
            {
                LOG_WARN(symbol + " PriceOffsetKind should be 1 or 2 ,now is: " + std::to_string(PriceOffsetKind));
                continue;
            }
            if (PriceOffset < 0 || PriceOffset >= 1)
            {
                LOG_WARN(symbol + " PriceOffset should in (0, 1),now is: " + std::to_string(PriceOffset));
                continue;
            }

            if (AmountOffsetKind != 1 && AmountOffsetKind != 2) 
            {
                LOG_WARN(symbol + " AmountOffsetKind should be 1 or 2 ,now is: " + std::to_string(AmountOffsetKind));
                continue;
            }

            if (AmountOffset < 0 || AmountOffset >= 1)
            {
                LOG_WARN(symbol + " AmountOffset should in (0, 1),now is: " + std::to_string(AmountOffset));
                continue;
            }            

            if (OTCOffsetKind != 1 && OTCOffsetKind != 2) 
            {
                LOG_WARN(symbol + " OTCOffsetKind should be 1 or 2 ,now is: " + std::to_string(OTCOffsetKind));
                continue;
            }

            if (OtcOffset < 0 || OtcOffset >= 1) 
            {
                LOG_WARN(symbol + " OtcOffset should in (0, 1) ,now is: " + std::to_string(PriceOffsetKind));
                continue;
            }                        

            if( symbol == "")
            {
                LOG_WARN("symbol is empty");
                continue;
            }            

            MarketRiskConfig cfg;

            cfg.symbol = symbol;
            cfg.PublishFrequency = PublishFrequency;
            cfg.PublishLevel = PublishLevel;
            cfg.PriceOffsetKind = PriceOffsetKind;
            cfg.PriceOffset = PriceOffset;
            cfg.AmountOffsetKind = AmountOffsetKind;
            cfg.AmountOffset = AmountOffset;
            cfg.DepositFundRatio = DepositFundRatio;

            cfg.OTCOffsetKind = OTCOffsetKind;
            cfg.OtcOffset = OtcOffset;
            cfg.IsPublish = IsPublish;

            output[symbol] = cfg;

            LOG_INFO("\nMarketRisk: " + symbol + "\n" + cfg.desc());
        }

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
    return true;
}

void ConfigurationClient::_parse_config()
{
    Document riskParamsObject;
    riskParamsObject.Parse(risk_params_.c_str());
    if(riskParamsObject.HasParseError())
    {
        LOG_ERROR("parse RiskParams error " + std::to_string(riskParamsObject.GetParseError()));
        return;
    }
    LOG_INFO("\nMarketRisk OriInfo: \n" + risk_params_.c_str());

    // 合并为内置配置格式
    map<TSymbol, MarketRiskConfig> output;
    if( combine_config(riskParamsObject, output) ) 
    {
        LOG_INFO("combine successfully");
        callback_->on_configuration_update(output);
    }
    else
    {
        LOG_WARN("Combine Failed!");
    }
}

bool ConfigurationClient::check_symbol(string symbol)
{
    // std::lock_guard<std::mutex> lk(risk_config_mutex_);

    // bool result = false;

    // // std::cout << "ConfigurationClient::check_symbol " << risk_config_.size() << std::endl;
    // // for(auto iter:risk_config_)
    // // {
    // //     std::cout << iter.second.desc() << std::endl;
    // // }

    // if (risk_config_.find(symbol) != risk_config_.end())
    // {
    //     // std::cout << "risk_config_ " << symbol << " , " << risk_config_[symbol].IsPublish << std::endl;
    //     return risk_config_[symbol].IsPublish;
    // }
    // else
    // {
    //     return false;
    // }    

    return false;
}