#include "nacos_client.h"
#include "stream_engine_config.h" // for _log_and_print
/*
NacosListener::NacosListener(ConfigService* server, string group, string dataid, INacosCallback* callback) 
{
    group_ = group;
    dataid_ = dataid;
    server_ = server;
    callback_ = callback;

    NacosString ss = "";
    try {
        ss = server_->getConfig(dataid_, group_, 1000);
    }
    catch (NacosException &e) {
        cout <<
            "Request failed with curl code:" << e.errorcode() << endl <<
            "Reason:" << e.what() << endl;
        return;
    }
    on_get_config(ss);
}

void NacosListener::on_get_config(const NacosString &configInfo) const
{
    callback_->on_config_channged(configInfo);
}

void NacosListener::receiveConfigInfo(const NacosString &configInfo) {
    on_get_config(configInfo);
}
*/
/*
void NacosClient::start(const string& addr, const string& group, const string& dataid, INacosCallback* callback) 
{
    addr_ = addr;
    group_ = group;
    dataid_ = dataid;

    _log_and_print("connect nacos addr=%s group=%s dataid=%s", addr_, group_, dataid_);
    _run_thread_ = new std::thread(&NacosClient::_run, this, callback);
}
*/

void NacosClient::start(const string& addr, const string& ns, INacosCallback* callback) 
{
    thread_run_ = true;

    addr_ = addr;
    namespace_ = ns;
    callback_ = callback;

    _log_and_print("connect nacos addr=%s namespace=%s", addr_, namespace_);
    _run_thread_ = new std::thread(&NacosClient::_run, this, callback);
}

void NacosClient::_run(INacosCallback* callback)
{
    Properties props;
    props[PropertyKeyConst::SERVER_ADDR] = addr_;
    props[PropertyKeyConst::NAMESPACE] = namespace_;
    NacosServiceFactory *factory = new NacosServiceFactory(props);
    ResourceGuard <NacosServiceFactory> _guardFactory(factory);
    server_ = factory->CreateConfigService();
    ResourceGuard <ConfigService> _serviceFactory(server_);

    while( thread_run_ ) {
        this->_request_configs();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool request_helper(ConfigService* service, const string& group, const string& dataid, NacosString& last, NacosString& raw)
{
    try {
        raw = service->getConfig(dataid, group, 1000);
    }
    catch (NacosException &e) {
        _log_and_print("request %s-%s errcode: %d Reason: %s", group, dataid, e.errorcode(), e.what());
        return false;
    }

    if( raw == last ){
        return false;
    }

    /*
    obj.Parse(raw.c_str());
    if(obj.HasParseError())
    {
        _log_and_print("parse %s-%s error", group, dataid, obj.GetParseError());
        return false;
    }    
    */
    _log_and_print("%s-%s: %s", group, dataid, raw);
    last = raw;
    return true;
}

uint32 double_to_uint(double val)
{
    uint32 ret = 0;
    double cost = val;
    while( cost < 1 ) {
        cost *= 10;
        ret += 1;
    }
    return ret;
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

bool helper_get_bool(const Value& v, const string& key, bool default_value)
{
    if( !v.HasMember(key.c_str()) || !v[key.c_str()].IsBool() )
        return default_value;
    return v[key.c_str()].GetBool();
}

string helper_get_string(const Value& v, const string& key, string default_value)
{
    if( !v.HasMember(key.c_str()) || !v[key.c_str()].IsString() )
        return default_value;
    return v[key.c_str()].GetString();
}

double helper_get_double(const Value& v, const string& key, double default_value)
{
    if( !v.HasMember(key.c_str()) || !v[key.c_str()].IsDouble() )
        return default_value;
    return v[key.c_str()].GetDouble();
}

uint32 helper_get_uint32(const Value& v, const string& key, uint32 default_value)
{
    if( !v.HasMember(key.c_str()) || !v[key.c_str()].IsUint() )
        return default_value;
    return v[key.c_str()].GetUint();
}

bool combine_config(const Document& hedges, const Document& symbols, Document& output)
{
    rapidjson::Document::AllocatorType& allocator = output.GetAllocator();

    // add symbol-config to output first
    if( !symbols.IsArray() )
        return false;        
    for( auto iter = symbols.Begin() ; iter != symbols.End() ; iter++ ) {
        string symbol = helper_get_string(*iter, "symbol_id", "");
        bool enable = helper_get_bool(*iter, "switch", true);
        uint32 volume_unit = double_to_uint(helper_get_double(*iter, "min_unit", 1));
        uint32 price_unit = double_to_uint(helper_get_double(*iter, "min_change_price", 1));
        uint32 frequency = helper_get_uint32(*iter, "frequency", 5);
        uint32 depth = helper_get_uint32(*iter, "depth", 100);

        if( symbol == "" || !enable )
            continue;

        Value symbol_config(rapidjson::Type::kObjectType);
        symbol_config.AddMember("enable", true, allocator);
        symbol_config.AddMember("precise", price_unit, allocator);
        symbol_config.AddMember("vprecise", volume_unit, allocator);
        symbol_config.AddMember("frequency", frequency, allocator);
        symbol_config.AddMember("depth", depth, allocator);
        symbol_config.AddMember("exchanges", Value(rapidjson::Type::kObjectType), allocator);

        Value symbol_key(symbol.c_str(), symbol.size(), allocator);
        output.AddMember(symbol_key, symbol_config, allocator);
    }

    // add exchange-config to output[symbol]
    if( !hedges.IsArray() )
        return false;
    for( auto iter = hedges.Begin() ; iter != hedges.End() ; iter++ ) {
        string exchange = helper_get_string(*iter, "platform_id", "");
        string symbol = helper_get_string(*iter, "instrument", "");
        bool enable = helper_get_bool(*iter, "switch", true);
        uint32 volume_unit = double_to_uint(helper_get_double(*iter, "min_unit", 1));
        uint32 price_unit = double_to_uint(helper_get_double(*iter, "min_change_price", 1));
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
        exchange_config.AddMember("precise", price_unit, allocator);
        exchange_config.AddMember("vprecise", volume_unit, allocator);
        exchange_config.AddMember("frequency", frequency, allocator);  // 暂时写死
        exchange_config.AddMember("fee_type", fee_type, allocator);
        exchange_config.AddMember("fee_maker", fee_maker, allocator);
        exchange_config.AddMember("fee_taker", fee_taker, allocator);

        Value exchange_key(exchange.c_str(), exchange.size(), allocator);
        output[symbol.c_str()]["exchanges"].AddMember(exchange_key, exchange_config, allocator);
    }

    return true;
}

void NacosClient::_request_configs()
{
    NacosString hedgeParams, symbolParams;
    Document hedgeParamsObject, symbolParamsObject;

    if( (request_helper(server_, "BCTS", "HedgeParams", hedge_params_, hedgeParams) || 
        request_helper(server_, "BCTS", "SymbolParams", symbol_params_, symbolParams)) &&
        hedge_params_ != "" && symbol_params_ != "" 
    ) {        
        hedgeParamsObject.Parse(hedge_params_.c_str());
        if(hedgeParamsObject.HasParseError())
        {
            _log_and_print("parse HedgeParams error %d", hedgeParamsObject.GetParseError());
            return;
        }
        symbolParamsObject.Parse(symbol_params_.c_str());
        if(symbolParamsObject.HasParseError())
        {
            _log_and_print("parse SymbolParams error %d", symbolParamsObject.GetParseError());
            return;
        }
        // 合并为内置配置格式
        Document output(rapidjson::Type::kObjectType);
        if( combine_config(hedgeParamsObject, symbolParamsObject, output) ) {
            callback_->on_config_channged(output);
        }
    }
}