#include "stream_engine.h"
#include "stream_engine_config.h"

// config file relative path
const char* config_file = "config.json";

StreamEngine::StreamEngine(){
    
    // init configuration
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    // init grpc server
    server_endpoint_.init(CONFIG->grpc_publish_addr_);
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() 
{
    quote_mixer2_.register_callback(&server_endpoint_);

    if( !CONFIG->replay_mode_ ) {
        // 启动redis
        quote_source_.set_engine(this);
        RedisParams params;
        params.host = CONFIG->quote_redis_host_;
        params.port = CONFIG->quote_redis_port_;
        params.password = CONFIG->quote_redis_password_;
        quote_source_.start(params, CONFIG->logger_);

        // 启动录数据线程
        quote_dumper_.start();

        // 启动db线程
        kline_db_.start();

        // 启动聚合K线计算线程
        kline_mixer_.register_callback(&kline_db_);
        kline_mixer_.register_callback(&server_endpoint_);
        kline_mixer_.set_db_interface(&kline_db_);
        kline_mixer_.start();
    } else {
        quote_replay_.set_engine(this);
        quote_replay_.start(CONFIG->replay_ratio_);
    }

    // start grpc server
    server_endpoint_.start();
    //PUBLISHER->run_in_thread();

    // 连接配置服务器
    nacos_client_.start(CONFIG->nacos_addr_, this);    
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    if( !CONFIG->replay_mode_ && CONFIG->dump_binary_ ) {
        quote_dumper_.on_snap(exchange, symbol, quote);
    }
    
    if( CONFIG->publish_data_ ) {
        quote_mixer2_.on_snap(exchange, symbol, quote);
    }
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    if( !CONFIG->replay_mode_ && CONFIG->dump_binary_ ) {
        quote_dumper_.on_update(exchange, symbol, quote);
    }

    if( CONFIG->publish_data_ ) {
        quote_mixer2_.on_update(exchange, symbol, quote);
    }
};

void StreamEngine::on_nodata_exchange(const TExchange& exchange) 
{
    quote_mixer2_.clear_exchange(exchange);
}

void StreamEngine::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline)
{
    kline_mixer_.on_kline(exchange, symbol, resolution, kline);
}

void StreamEngine::signal_handler(int signum)
{
    UT_LOG_INFO(CONFIG->logger_, "StreamEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    // 退出
    exit(0);
} 

map<TExchange, SymbolFee> to_fees(const map<TExchange, SNacosConfigFee>& exchanges) {    
    map<TExchange, SymbolFee> fees;
    for( const auto& v : exchanges ) {
        SymbolFee tmp;
        tmp.fee_type = v.second.fee_type;
        tmp.maker_fee = v.second.fee_maker;
        tmp.taker_fee = v.second.fee_taker;
        fees[v.first] = tmp;
    }
    return fees;
}

bool map_equal(const map<TExchange, SymbolFee>& m1, const map<TExchange, SymbolFee>& m2) {
    for( const auto& v : m1 ) {
        auto iter = m2.find(v.first);
        if( iter == m2.end() )
            return false;
        const SymbolFee& f1 = v.second;
        const SymbolFee& f2 = iter->second;
        if( memcmp(&f1, &f2, sizeof(SymbolFee)) != 0 )
            return false;
    }
    for( const auto& v : m2 ) {
        if( m1.find(v.first) == m1.end() )
            return false;
    }
    return true;
}

void StreamEngine::on_symbol_channged(const NacosString& configInfo)
{
    njson js;
    
    try{
        js = njson::parse(configInfo);
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("parse json fail %s", e.what());
        return;
    }

    std::unordered_map<TSymbol, SNacosConfig> symbols;
    for (auto iter = js.begin() ; iter != js.end() ; ++iter )
    {
        const TSymbol& symbol = iter.key();
        const njson& symbol_cfgs = iter.value();
        int enable = symbol_cfgs["enable"].get<int>();
        if( enable < 1 )
            continue;
        SNacosConfig cfg;
        cfg.precise = symbol_cfgs["precise"].get<int>();
        //cfg.vprecise = symbol_cfgs["vprecise"].get<int>();
        cfg.depth = symbol_cfgs["depth"].get<unsigned int>();
        cfg.frequency = symbol_cfgs["frequency"].get<float>();
        cfg.mix_depth = symbol_cfgs["mix_depth"].get<unsigned int>();
        cfg.mix_frequecy = symbol_cfgs["mix_frequency"].get<float>();
        for( auto iter2 = symbol_cfgs["exchanges"].begin() ; iter2 != symbol_cfgs["exchanges"].end() ; ++iter2 )
        {
            const TExchange& exchange = iter2.key();
            const njson& exchange_cfgs = iter2.value();
            SNacosConfigFee exchange_cfg;
            exchange_cfg.fee_type = exchange_cfgs["fee_type"].get<int>();
            exchange_cfg.fee_maker = exchange_cfgs["fee_maker"].get<float>();
            exchange_cfg.fee_taker = exchange_cfgs["fee_taker"].get<float>();
            cfg.exchanges[exchange] = exchange_cfg;
        } 
        symbols[symbol] = cfg;
    }
    
    // quote_source_ 涉及交易所+交易币种，原始行情更新频率
    // quote_mixer2_ 涉及交易币种的精度，深度，频率，各交易所手续费
    for( const auto& v : symbols )
    {
        const TSymbol& symbol = v.first;
        const SNacosConfig& config = v.second;
        // 新增品种
        if( symbols_.find(symbol) == symbols_.end() ) 
        {
            quote_mixer2_.set_compute_params(symbol, config.precise, config.mix_depth, to_fees(config.exchanges));
            quote_mixer2_.set_publish_params(symbol, config.mix_frequecy);
            kline_mixer_.set_symbol(symbol, config.get_exchanges());
            quote_source_.set_frequency(symbol, config.frequency);
            quote_source_.set_symbol(symbol, config.get_exchanges());
        }
        // 已有的品种
        else 
        { 
            const SNacosConfig& last_config = symbols_[symbol];
            // 行情原始频率变更
            if( last_config.frequency != config.frequency ) {
                quote_source_.set_frequency(symbol, config.frequency);
            }
            // 交易所数量变更
            if( last_config.get_exchanges() != config.get_exchanges() ) {
                kline_mixer_.set_symbol(symbol, config.get_exchanges());
                quote_source_.set_symbol(symbol, config.get_exchanges());
            }
            // 精度，各交易所手续费 
            if( last_config.precise != config.precise || !map_equal(to_fees(last_config.exchanges), to_fees(config.exchanges)) || last_config.mix_depth != config.mix_depth) {
                quote_mixer2_.set_compute_params(symbol, config.precise, config.mix_depth, to_fees(config.exchanges));
            }
            // 频率和深度
            if( last_config.mix_frequecy != config.mix_frequecy ) {
                quote_mixer2_.set_publish_params(symbol, config.mix_frequecy);
            }
        }
    }


    // 删除品种
    for( const auto& v : symbols_ ) {
        if( symbols.find(v.first) == symbols.end() ) {
            quote_source_.set_symbol(v.first, unordered_set<TExchange>());
        }
    }

    // 赋值
    symbols_ = symbols;

    CONFIG->set_config(configInfo);
}