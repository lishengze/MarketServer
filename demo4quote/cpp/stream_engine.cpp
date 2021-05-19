#include "stream_engine.h"
#include "stream_engine_config.h"
#include "redis_quote.h"

#define MODE_REALTIME "realtime"
#define MODE_REPLAY "replay"
 
StreamEngine::StreamEngine()
{
    // 设置聚合K线计算回调
    kline_mixer_.set_engine(this);

    // 设置聚合行情计算回调
    quote_mixer2_.set_engine(this);

    // K线缓存设置
    kline_hubber_.register_callback(&server_endpoint_);
    kline_hubber_.set_db_interface(&kline_db_);

    // 行情缓存设置
    quote_cacher_.register_callback(&server_endpoint_);
    
    // 初始化服务端
    server_endpoint_.set_cacher(&kline_hubber_); // 必须在init之前
    server_endpoint_.set_quote_cacher(&quote_cacher_); // 必须在init之前
    server_endpoint_.init(CONFIG->grpc_publish_addr_);
}

StreamEngine::~StreamEngine()
{
}

void StreamEngine::init()
{
    _log_and_print("run mode %s", CONFIG->mode_);
    if( CONFIG->mode_ == MODE_REALTIME ) {
        RedisParams params;
        params.host = CONFIG->quote_redis_host_;
        params.port = CONFIG->quote_redis_port_;
        params.password = CONFIG->quote_redis_password_;
        RedisQuote* ptr = new RedisQuote();
        ptr->init(this, params, CONFIG->logger_, CONFIG->dump_);
        quote_source_ = ptr;
    } else if( CONFIG->mode_ == MODE_REPLAY ) {
        RedisQuote* ptr = new RedisQuote();
        ptr->init_replay(this, CONFIG->replay_ratio_, CONFIG->replay_replicas_);
        quote_source_ = ptr;
    } else {
        _log_and_print("unknown mode %s", CONFIG->mode_);
    }
}

void StreamEngine::start() 
{
    // 启动行情聚合线程
    quote_mixer2_.start();

    // 启动db写入
    kline_db_.start();

    // 启动grpc服务器
    server_endpoint_.start();

    // 连接配置服务器
    config_client_.set_callback(this);
    std::cout << "Nacos Address: " << CONFIG->nacos_addr_ << " name: " << CONFIG->nacos_namespace_ << std::endl;
    config_client_.start(CONFIG->nacos_addr_, CONFIG->nacos_namespace_);    

    kline_hubber_.recover_from_db();
}

void StreamEngine::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote)
{
    if( string(exchange) == MIX_EXCHANGE_NAME && string(symbol) == "BTC_USDT" ) {
    //    tfm::printfln("StreamEngine: %s.%s ask/bid %lu/%lu", exchange, symbol, quote.asks.size(), quote.bids.size());
    //    std::cout << "StreamEngine::on_snap " << exchange << " " << symbol << " " << quote.asks.size() << "/" << quote.bids.size() << std::endl;
    }
    quote_cacher_.on_snap(exchange, symbol, quote);

    if( exchange != MIX_EXCHANGE_NAME  ) {
        quote_mixer2_.on_snap(exchange, symbol, quote);
    }
};

void StreamEngine::on_update(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote)
{
    SDepthQuote snap; // snap为增量更新后得到的快照
    quote_cacher_.on_update(exchange, symbol, quote, snap);
    if( string(exchange) == "BINANCE" && string(symbol) == "BTC_USDT" ) {
    //    _print("update: %s.%s %s bias=%lu", exchange, symbol, FormatISO8601DateTime(quote.origin_time/1000000000), get_miliseconds()/1000 - quote.origin_time/1000000000);
    //    tfm::printfln("update to snap: %s.%s ask/bid %lu/%lu", exchange, symbol, snap.asks.size(), snap.bids.size());
    }

    if( exchange != MIX_EXCHANGE_NAME  ) {
        quote_mixer2_.on_snap(exchange, symbol, snap);
    }
};

void StreamEngine::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    quote_cacher_.on_trade(exchange, symbol, trade);

    if( exchange != MIX_EXCHANGE_NAME  ) {
        quote_mixer2_.on_trade(exchange, symbol, trade);
    }
}

void StreamEngine::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init)
{
    /**/
    for( const auto& v : klines ) {     
        // cout << "[kline] " << exchange << " " << symbol << " " << resolution << " " << klines.size() << endl;

        // _log_and_print("get %s.%s kline%d index=%lu open=%s high=%s low=%s close=%s volume=%s", exchange.c_str(), symbol.c_str(), resolution, 
        //     v.index,
        //     v.px_open.get_str_value().c_str(),
        //     v.px_high.get_str_value().c_str(),
        //     v.px_low.get_str_value().c_str(),
        //     v.px_close.get_str_value().c_str(),
        //     v.volume.get_str_value().c_str()
        // );
    }
    
    vector<KlineData> outputs; // 
    kline_hubber_.on_kline(exchange, symbol, resolution, klines, is_init, outputs);

    if( exchange != MIX_EXCHANGE_NAME  )
        kline_mixer_.on_kline(exchange, symbol, resolution, outputs, is_init);
}

void StreamEngine::on_nodata_exchange(const TExchange& exchange) 
{
    quote_cacher_.clear_exchange(exchange);
}

void StreamEngine::signal_handler(int signum)
{
    UT_LOG_INFO(CONFIG->logger_, "StreamEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    // 退出
    exit(0);
} 

QuoteMixer2::SMixerConfig to_mixer_config(type_uint32 depth, type_uint32 precise, type_uint32 vprecise, float frequency, const unordered_map<TExchange, SNacosConfigByExchange>& exchanges) 
{    
    QuoteMixer2::SMixerConfig config;
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

void expand_replay_config(const Document& src, Document& dst)
{
    if( CONFIG->replay_replicas_ == 0 ) {
        return;
    }

    // 待复制的代码
    set<TSymbol> symbols;
    for (auto iter = src.MemberBegin() ; iter != src.MemberEnd() ; ++iter )
    {
        const TSymbol& symbol = iter->name.GetString();
        const Value& symbol_cfgs = iter->value;
        bool enable = symbol_cfgs["enable"].GetBool();
        if( !enable )
            continue;
        symbols.insert(symbol);
    }

    // 执行复制
    for( const auto& v : symbols ) {     
        for( unsigned int i = 1 ; i <= CONFIG->replay_replicas_ ; i ++ ) {
            string key = tfm::format("%s_%d", v, i);
            Value symbol_key(key.c_str(), key.size(), dst.GetAllocator());
            Value value;
            value.CopyFrom(dst[v.c_str()], dst.GetAllocator());
            dst.AddMember(symbol_key, value, dst.GetAllocator());
        }
    }
}

void StreamEngine::on_config_channged(const Document& src)
{   
    /* 
    _log_and_print("parse json %s", configInfo);

    // json 解析
    njson js;    
    try
    {
        js = njson::parse(configInfo);
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("parse json fail %s", e.what());
        return;
    }    
    _log_and_print("parse config from nacos finish");
    */

    Document d(rapidjson::Type::kObjectType);
    string content = ToJson(src);
    d.Parse(content.c_str());
    // 压测扩展配置参数
    if( CONFIG->mode_ == MODE_REPLAY ) {
        expand_replay_config(src, d);
    }

    std::cout << "\n\n *** StreamEngine::on_config_channged ***" <<endl;

    // string -> 结构化数据
    std::unordered_map<TSymbol, SNacosConfig> symbols;
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
            symbols[symbol] = cfg;

            cout << "symbol: " << symbol << " \n"
                 << cfg.str() << endl;
        }
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("decode config fail %s", e.what());
        return;
    }    
    
    for( const auto& v : symbols )
    {
        const TSymbol& symbol = v.first;
        const SNacosConfig& config = v.second;

        // 新增品种
        if( symbols_.find(symbol) == symbols_.end() ) 
        {
            quote_mixer2_.set_config(symbol, to_mixer_config(config.depth, config.precise, config.vprecise, config.frequency, config.exchanges));
            kline_mixer_.set_symbol(symbol, config.get_exchanges());
            quote_source_->set_config(symbol, to_redis_config(config.exchanges));
        }
        // 已有的品种
        else 
        { 
            const SNacosConfig& last_config = symbols_[symbol];
            // 源头配置变更
            if( to_redis_config(last_config.exchanges) != to_redis_config(config.exchanges) ) {
                quote_source_->set_config(symbol, to_redis_config(config.exchanges));
            }
            // 交易所数量变更
            if( last_config.get_exchanges() != config.get_exchanges() ) {
                kline_mixer_.set_symbol(symbol, config.get_exchanges());
            }
            // mixer配置变更
            if( to_mixer_config(config.depth, config.precise, config.vprecise, config.frequency, config.exchanges) != 
                to_mixer_config(last_config.depth, last_config.precise, last_config.vprecise, last_config.frequency, last_config.exchanges) ) {
                quote_mixer2_.set_config(symbol, to_mixer_config(config.depth, config.precise, config.vprecise, config.frequency, config.exchanges));
            }
        }
    }


    // 删除品种
    for( const auto& v : symbols_ ) {
        if( symbols.find(v.first) == symbols.end() ) {
            quote_source_->set_config(v.first, SSymbolConfig());
        }
    }

    // 保存
    symbols_ = symbols;

    // 配置信息缓存在config中，供接口请求
    CONFIG->set_config(ToJson(d));
    
    // 启动数据接收
    quote_source_->start();
}