#include "configuration_client.h"
#include "stream_engine_config.h" // for _log_and_print


void ConfigurationClient::_run()
{
    add_listener("BCTS", "HedgeParams", hedger_watcher_);
    add_listener("BCTS", "SymbolParams", symbol_watcher_);
    add_listener("BCTS", "MarketRisk", risk_watcher_);

    while( is_running() ) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
//
// symbol -> {
//     enble
//     precise
//     vprecise
//     depth
//     frequency
//     exchange -> {
//         enable
//         precise
//         vprecise
//         frequency
//         fee_type
//         fee_maker
//         fee_taker
//     }   
// }

bool combine_config(const Document& hedges, const Document& symbols, const Document& risks, Document& output)
{
    try
    {
        rapidjson::Document::AllocatorType& allocator = output.GetAllocator();

        // add symbol-config to output first
        if( !symbols.IsArray() )
            return false;        
        for( auto iter = symbols.Begin() ; iter != symbols.End() ; iter++ ) {
            string symbol = helper_get_string(*iter, "symbol_id", "");
            bool enable = helper_get_bool(*iter, "switch", true);
            uint32 price_precision = helper_get_uint32(*iter, "price_precision", 0); // 价格精度
            uint32 amount_precision = helper_get_uint32(*iter, "amount_precision", 0); // 数量精度
            uint32 sum_precision = helper_get_uint32(*iter, "sum_precision", 0); // 金额精度

            if( symbol == "" || !enable )
                continue;

            Value symbol_config(rapidjson::Type::kObjectType);
            symbol_config.AddMember("enable", true, allocator);
            symbol_config.AddMember("precise", price_precision, allocator);
            symbol_config.AddMember("vprecise", amount_precision, allocator);
            symbol_config.AddMember("aprecise", sum_precision, allocator);
            symbol_config.AddMember("frequency", 5, allocator); // 添加默认值
            symbol_config.AddMember("depth", 100, allocator); // 添加默认值
            symbol_config.AddMember("exchanges", Value(rapidjson::Type::kObjectType), allocator);

            Value symbol_key(symbol.c_str(), symbol.size(), allocator);
            output.AddMember(symbol_key, symbol_config, allocator);
        }

        // add risk-config to ouput
        if( !risks.IsArray() )
            return false;   
        for( auto iter = risks.Begin() ; iter != risks.End() ; iter++ ) {
            string symbol = helper_get_string(*iter, "symbol_id", "");
            bool enable = helper_get_bool(*iter, "switch", true);
            uint32 frequency = helper_get_uint32(*iter, "publish_frequency", 5);
            uint32 depth = helper_get_uint32(*iter, "publish_level", 100);

            if( symbol == "" || !enable || !output.HasMember(symbol.c_str()))
                continue;

            output[symbol.c_str()]["frequency"] = frequency;
            output[symbol.c_str()]["depth"] = depth;
        }

        // add exchange-config to output[symbol]
        if( !hedges.IsArray() )
            return false;
        for( auto iter = hedges.Begin() ; iter != hedges.End() ; iter++ ) {
            string exchange = helper_get_string(*iter, "platform_id", "");
            string symbol = helper_get_string(*iter, "instrument", "");
            bool enable = helper_get_bool(*iter, "switch", true);
            //uint32 volume_unit = double_to_uint(helper_get_double(*iter, "min_unit", 1));
            //uint32 price_unit = double_to_uint(helper_get_double(*iter, "min_change_price", 1));
            uint32 price_precision = helper_get_uint32(*iter, "price_precision", 0); // 价格精度
            uint32 amount_precision = helper_get_uint32(*iter, "amount_precision", 0); // 数量精度
            uint32 sum_precision = helper_get_uint32(*iter, "sum_precision", 0); // 金额精度（暂时没用）
            uint32 fee_type = helper_get_uint32(*iter, "fee_kind", 0);
            uint32 fee_taker = helper_get_uint32(*iter, "taker_fee", 0);
            uint32 fee_maker = helper_get_uint32(*iter, "maker_fee", 0);
            uint32 frequency = helper_get_uint32(*iter, "frequency", 0);

            if( exchange == "" || symbol == "" || !enable )
                continue;
            if( !output.HasMember(symbol.c_str()) )
                continue;

            Value exchange_config(rapidjson::Type::kObjectType);
            exchange_config.AddMember("enable", true, allocator);
            exchange_config.AddMember("precise", price_precision, allocator);
            exchange_config.AddMember("vprecise", amount_precision, allocator);
            exchange_config.AddMember("aprecise", sum_precision, allocator);
            exchange_config.AddMember("frequency", frequency, allocator);  // 暂时写死
            exchange_config.AddMember("fee_type", fee_type, allocator);
            exchange_config.AddMember("fee_maker", fee_maker, allocator);
            exchange_config.AddMember("fee_taker", fee_taker, allocator);

            Value exchange_key(exchange.c_str(), exchange.size(), allocator);
            output[symbol.c_str()]["exchanges"].AddMember(exchange_key, exchange_config, allocator);
        }

        return true;
    }
    catch(const std::exception& e)
    {
        _log_and_print("[Exception] %s ", e.what());
    }
    

}

void ConfigurationClient::config_changed(const string& group, const string& dataid, const NacosString &configInfo)
{
    try
    {
        if( group == "BCTS" && dataid == "HedgeParams" ) {
            hedge_params_ = configInfo;
        } else if( group == "BCTS" && dataid == "SymbolParams" ) {
            symbol_params_ = configInfo;
        } else if( group == "BCTS" && dataid == "MarketRisk" ) {
            risk_params_ = configInfo;
        } else {
            _log_and_print("unknown configuration.");
            return;
        }
        
        if( hedge_params_.length() > 0 && symbol_params_.length() > 0 && risk_params_.length() > 0 ) {
            this->_parse_config();
        }
    }
    catch(const std::exception& e)
    {
        _log_and_print("[Exception] %s ", e.what());
    }
    
    // _log_and_print("nacos configuration %s-%s %s", group, dataid, configInfo);
}

void ConfigurationClient::_parse_config()
{
    try
    {
        Document hedgeParamsObject;    
        hedgeParamsObject.Parse(hedge_params_.c_str());
        if(hedgeParamsObject.HasParseError())
        {
            _log_and_print("parse HedgeParams error %d", hedgeParamsObject.GetParseError());
            return;
        }
            
        Document symbolParamsObject;
        symbolParamsObject.Parse(symbol_params_.c_str());
        if(symbolParamsObject.HasParseError())
        {
            _log_and_print("parse SymbolParams error %d", symbolParamsObject.GetParseError());
            return;
        }
            
        Document riskParamsObject;
        riskParamsObject.Parse(risk_params_.c_str());
        if(riskParamsObject.HasParseError())
        {
            _log_and_print("parse RiskParams error %d", riskParamsObject.GetParseError());
            return;
        }


        std::cout << "\nhedgeParamsObject: \n " << ToJson(hedgeParamsObject) << "\n"
                << "\nsymbolParamsObject: \n " << ToJson(symbolParamsObject) << "\n"
                << "\nriskParamsObject: \n " << ToJson(riskParamsObject) << "\n"
                << std::endl;

        // 合并为内置配置格式
        Document output(rapidjson::Type::kObjectType);
        if( combine_config(hedgeParamsObject, symbolParamsObject, riskParamsObject, output) ) {
            callback_->on_config_channged(output);
        }
    }
    catch(const std::exception& e)
    {
        _log_and_print("[Exception] %s ", e.what());
    }
}