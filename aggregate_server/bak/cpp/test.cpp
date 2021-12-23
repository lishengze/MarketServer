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
        p_encode_processer_ = new EncodeProcesser();

        p_depth_processor_ = new DepthProcessor(p_encode_processer_);
        p_kline_processor_ = new KlineProcessor(p_encode_processer_);
        p_trade_processor_ = new TradeProcessor(p_encode_processer_);
        
        p_decode_processer_ = new DecodeProcesser(p_depth_processor_, p_kline_processor_, 
                                                  p_trade_processor_, engine_pool_);

        p_kafka_ = new KafkaServer(p_decode_processer_);
        p_encode_processer_->set_kafka_server(p_kafka_);

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

SMixerConfig to_mixer_config(const SNacosConfig& nano_config) 
{    
    SMixerConfig config;
    config.depth = nano_config.depth;
    config.precise = nano_config.precise;
    config.vprecise = nano_config.vprecise;
    config.frequency = nano_config.frequency;
    for( const auto& v : nano_config.exchanges ) 
    {
        config.fees[v.first] = v.second.fee;
    }
    return config;
}

std::set<TExchange> get_exchange_set(const SNacosConfig& config)
{
    try
    {
        std::set<TExchange> result;

        for (auto iter:config.exchanges)
        {
            result.emplace(iter.first);
        }
        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

bool TestEngine::parse_config(const Document& src, std::unordered_map<TSymbol, SNacosConfig>& curr_config)
{
    try
    {
        Document d(rapidjson::Type::kObjectType);
        string content = ToJson(src);
        d.Parse(content.c_str());

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
            // LOG_INFO("\n" + symbol + ": " + cfg.str());
        }

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }

    return false;
}

void TestEngine::update_config(const std::unordered_map<TSymbol, SNacosConfig>& curr_config, bool& is_config_changed, bool& is_meta_changed)
{
    try
    {
        for( const auto& v : curr_config )
        {
            const TSymbol& symbol = v.first;
            const SNacosConfig& config = v.second;

            SMixerConfig mixer_config = to_mixer_config(config);
            std::set<TExchange> exchange_set = get_exchange_set(config);

            // 新增品种
            if( nacos_config_.find(symbol) == nacos_config_.end() ) 
            {
                meta_map_[symbol] = exchange_set;
                mixer_config_[symbol] = mixer_config;

                is_config_changed = true;
                is_meta_changed = true;
            }
            // 已有的品种
            else 
            { 
                if (mixer_config != to_mixer_config(nacos_config_[symbol]))
                {
                    is_config_changed = true;
                    mixer_config_[symbol] = mixer_config;
                }
            }            
        }

        // 删除品种
        for( const auto& v : nacos_config_ ) 
        {
            if( curr_config.find(v.first) == curr_config.end() ) 
            {
                is_config_changed = true;
                if (mixer_config_.find(v.first) != mixer_config_.end())
                {
                    mixer_config_.erase(v.first);
                }

                is_meta_changed = true;
                if (meta_map_.find(v.first) != meta_map_.end())
                {
                    meta_map_.erase(v.first);
                }
            }
        }

        nacos_config_ = curr_config;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void TestEngine::on_config_channged(const Document& src)
{
    try
    {
        LOG_INFO("*** TestEngine::on_config_channged ***");

        std::unordered_map<TSymbol, SNacosConfig> curr_config;

        if (!parse_config(src, curr_config)) return;

        bool is_config_changed = false;
        bool is_meta_changed = false;

        update_config(curr_config, is_config_changed, is_meta_changed);

        if (is_config_changed) notify_config_change();

        if (is_meta_changed) notify_meta_change();

    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}


void TestEngine::notify_config_change()
{
    try
    {
        LOG_INFO("mixer_config: ");
        for (auto iter:mixer_config_)
        {
            LOG_INFO(iter.first + " " + iter.second.simple_str());
        }

        p_depth_processor_->set_config(mixer_config_);
        p_kline_processor_->set_config(mixer_config_);
        p_trade_processor_->set_config(mixer_config_);        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void TestEngine::notify_meta_change()
{
    try
    {
        LOG_INFO("meta_map: ");
        for (auto iter:meta_map_)
        {
            for (auto exchange:iter.second)
            {
                LOG_INFO(iter.first + "."+ exchange);
            }
        }

        p_kafka_->set_meta(meta_map_);
        p_decode_processer_->set_meta(meta_map_);
        p_kline_processor_->set_meta(meta_map_);
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