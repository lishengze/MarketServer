#include "test.h"
#include "global_declare.h"
#include "Log/log.h"
#include "pandora/util/io_service_pool.h"
#include "stream_engine_config.h"

TestEngine::~TestEngine()
{
    if (p_kafka_)
    {
        delete p_kafka_;
        p_kafka_ = nullptr;
    }

    if (p_decode_processer_)
    {
        delete p_decode_processer_;
        p_decode_processer_ = nullptr;
    }
}

void TestEngine::init()
{
    try
    {
        p_decode_processer_ = new DecodeProcesser(engine_pool_);

        p_kafka_ = new KafkaServer(p_decode_processer_);

        config_client_.set_callback(this);

        LOG_INFO(string("Nacos Address: ") + CONFIG->nacos_addr_ + " namespace: " + CONFIG->nacos_namespace_);

        config_client_.start(CONFIG->nacos_addr_, CONFIG->nacos_namespace_);    
        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }  
}

void TestEngine::start()
{
    try
    {
        p_kafka_->launch();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

SSymbolConfig to_redis_config(const unordered_map<TExchange, SNacosConfigByExchange>& configs)
{
    SSymbolConfig ret;
    for( const auto& v : configs ) {
        ret[v.first].enable = true;
        ret[v.first].precise = v.second.precise;
        ret[v.first].vprecise = v.second.vprecise;
        ret[v.first].aprecise = v.second.aprecise;
        ret[v.first].frequency = v.second.frequency;
    }
    return ret;
}

SMixerConfig to_mixer_config(type_uint32 depth, type_uint32 precise, type_uint32 vprecise, float frequency, 
                            const unordered_map<TExchange, SNacosConfigByExchange>& exchanges) 
{    
    SMixerConfig config;
    config.depth = depth;
    config.precise = precise;
    config.vprecise = vprecise;
    config.frequency = frequency;
    for( const auto& v : exchanges ) 
    {
        config.fees[v.first] = v.second.fee;
    }
    return config;
}

void TestEngine::on_config_channged(const Document& src)
{
    try
    {
        Document d(rapidjson::Type::kObjectType);
        string content = ToJson(src);
        d.Parse(content.c_str());

        LOG_INFO("*** TestEngine::on_config_channged ***");
        LOG_INFO(content);

        // string -> 结构化数据
        std::unordered_map<TSymbol, SNacosConfig> curr_config;
        try
        {
            for (auto iter = d.MemberBegin() ; iter != d.MemberEnd() ; ++iter )
            {
                const TSymbol& symbol = iter->name.GetString();
                const Value& symbol_cfgs = iter->value;
                bool enable = symbol_cfgs["enable"].GetBool();
                if( !enable )
                    continue;
                SNacosConfig cfg;
                cfg.precise = symbol_cfgs["precise"].GetUint();
                cfg.vprecise = symbol_cfgs["vprecise"].GetUint();
                cfg.aprecise = symbol_cfgs["aprecise"].GetUint();
                cfg.depth = symbol_cfgs["depth"].GetUint();
                cfg.frequency = symbol_cfgs["frequency"].GetFloat();
                for( auto iter2 = symbol_cfgs["exchanges"].MemberBegin() ; iter2 != symbol_cfgs["exchanges"].MemberEnd() ; ++iter2 )
                {
                    const TExchange& exchange = iter2->name.GetString();
                    const Value& exchange_cfgs = iter2->value;
                    SNacosConfigByExchange exchange_cfg;
                    exchange_cfg.precise = exchange_cfgs["precise"].GetUint();
                    exchange_cfg.vprecise = exchange_cfgs["vprecise"].GetUint();
                    //exchange_cfg.depth = exchange_cfgs["depth"].get<int>();
                    exchange_cfg.frequency = exchange_cfgs["frequency"].GetFloat();
                    exchange_cfg.fee.fee_type = exchange_cfgs["fee_type"].GetUint();
                    exchange_cfg.fee.maker_fee = exchange_cfgs["fee_maker"].GetDouble();
                    exchange_cfg.fee.taker_fee = exchange_cfgs["fee_taker"].GetDouble();
                    cfg.exchanges[exchange] = exchange_cfg;
                } 
                curr_config[symbol] = cfg;

                LOG_INFO("\n" + symbol + ": " + cfg.str());

            }
        }
        catch(nlohmann::detail::exception& e)
        {
            LOG_WARN("decode config fail " + e.what());
            
            return;
        }    

        bool is_config_changed = false;
        
        std::map<string, SSymbolConfig> updated_symbol_map;
        for( const auto& v : curr_config )
        {
            const TSymbol& symbol = v.first;
            const SNacosConfig& config = v.second;

            SSymbolConfig symbol_config = to_redis_config(config.exchanges);
            SMixerConfig mixer_config = to_mixer_config(config.depth, config.precise, config.vprecise, config.frequency, config.exchanges);

            // 新增品种
            if( nacos_config_.find(symbol) == nacos_config_.end() ) 
            {
                trans_config_[symbol] = symbol_config;

                is_config_changed = true;
                // p_kafka_->set_config(symbol, symbol_config);
                // p_decode_processer_->set_config(symbol, symbol_config);
            }
            // 已有的品种
            else 
            { 
                const SNacosConfig& last_config = nacos_config_[symbol];
                // 源头配置变更
                if( to_redis_config(last_config.exchanges) != to_redis_config(config.exchanges) ) 
                {
                    is_config_changed = true;
                    trans_config_[symbol] = symbol_config;

                    // p_kafka_->set_config(symbol, symbol_config);
                    // p_decode_processer_->set_config(symbol, symbol_config);
                }
            }

            
        }


        // 删除品种
        for( const auto& v : nacos_config_ ) 
        {
            if( curr_config.find(v.first) == curr_config.end() ) 
            {
                is_config_changed = true;
                if (trans_config_.find(v.first) != trans_config_.end())
                {
                    trans_config_.erase(v.first);
                }

                // p_kafka_->set_config(v.first, SSymbolConfig());
                // p_decode_processer_->set_config(v.first, SSymbolConfig());
            }
        }

        // 保存
        nacos_config_ = curr_config;

        if (is_config_changed)
        {
            p_kafka_->set_new_config(trans_config_);
            p_decode_processer_->set_new_config(trans_config_);
        }

        // 启动数据接收
        // p_kafka_->launch();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}


void test_engine()
{
    string config_file_name = "config.json";
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file_name);

    utrade::pandora::io_service_pool engine_pool{3};

    TestEngine test_obj(engine_pool);
    test_obj.start();

    engine_pool.start();
    engine_pool.block();
}

void TestMain()
{
    test_engine();
}