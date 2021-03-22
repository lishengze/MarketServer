#include "updater_configuration.h"
#include "risk_controller_config.h"

void ConfigurationClient::_run()
{
    add_listener("BCTS", "MarketRisk", risk_watcher_);

    while( is_running() ) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ConfigurationClient::config_changed(const string& group, const string& dataid, const NacosString &configInfo)
{
    _log_and_print("nacos configuration %s-%s %s", group, dataid, configInfo);
    
    if( group == "BCTS" && dataid == "MarketRisk" ) {
        risk_params_ = configInfo;
    } else {
        _log_and_print("unknown configuration.");
        return;
    }
    
    if( risk_params_.length() > 0 ) {
        this->_parse_config();
    }
}

bool combine_config(const Document& risks, map<TSymbol, QuoteConfiguration>& output)
{    
    try
    {
        std::cout << "combine_config " << std::endl;

        // add symbol-config to output first
        if( !risks.IsArray() )
            return false;        
        for( auto iter = risks.Begin() ; iter != risks.End() ; iter++ ) {
            string symbol = helper_get_string(*iter, "symbol_id", "");
            bool enable = helper_get_bool(*iter, "switch", true);
            uint32 price_bias_kind = helper_get_uint32(*iter, "price_offset_kind", 1); // 暂时没用
            double price_bias = helper_get_double(*iter, "price_offset", 0);
            uint32 volume_bias_kind = helper_get_uint32(*iter, "amount_offset_kind", 1); // 暂时没用
            double volume_bias = helper_get_double(*iter, "amount_offset", 0);
            uint32 otc_bias_kind = helper_get_uint32(*iter, "poll_offset_kind", 1); // 暂时没用
            double otc_bias = helper_get_double(*iter, "poll_offset", 0);
            double deposit_percent = helper_get_double(*iter, "deposit_fund_ratio", 100); // 暂时没用
            double hedge_percent = helper_get_double(*iter, "hedge_fund_ratio", 100);

            // if( symbol == "" || !enable )
            //     continue;

            QuoteConfiguration cfg;
            cfg.PriceBias = price_bias;
            cfg.VolumeBias = volume_bias;
            cfg.HedgePercent = hedge_percent;
            cfg.OtcBias = otc_bias;
            output[symbol] = cfg;

            std::cout << symbol << " PriceBias: " << price_bias << " VolumeBias: " << volume_bias << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    


    return true;
}

void ConfigurationClient::_parse_config()
{
    Document riskParamsObject;
    riskParamsObject.Parse(risk_params_.c_str());

    std::cout << "risk_params_: " << risk_params_.c_str() << std::endl;

    if(riskParamsObject.HasParseError())
    {
        _log_and_print("parse RiskParams error %d", riskParamsObject.GetParseError());
        return;
    }

    // 合并为内置配置格式
    map<TSymbol, QuoteConfiguration> output;
    if( combine_config(riskParamsObject, output) ) {
        callback_->on_configuration_update(output);
    }
}
