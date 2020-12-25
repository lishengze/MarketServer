#include "stream_engine.h"
#include "stream_engine_config.h"

StreamEngine::StreamEngine(){

    // 
    quote_source_.set_engine(this);
    quote_replay_.set_engine(this);

    // 启动聚合K线计算线程
    kline_mixer_.set_engine(this);

    // 
    kline_hubber_.register_callback(&server_endpoint_);
    kline_hubber_.set_db_interface(&kline_db_);
    
    // init grpc server
    server_endpoint_.set_cacher(&kline_hubber_); // 必须在init之前
    server_endpoint_.init(CONFIG->grpc_publish_addr_);
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() 
{
    quote_mixer2_.register_callback(&server_endpoint_);

    if( !CONFIG->replay_mode_ ) {
        // 启动redis
        RedisParams params;
        params.host = CONFIG->quote_redis_host_;
        params.port = CONFIG->quote_redis_port_;
        params.password = CONFIG->quote_redis_password_;
        quote_source_.start(params, CONFIG->logger_);

        // 启动录数据线程
        quote_dumper_.start();

        // 启动db线程
        kline_db_.start();
        
    } else {
        // 
        quote_replay_.start(CONFIG->replay_ratio_);
    }

    // start grpc server
    server_endpoint_.start();

    // 连接配置服务器
    nacos_client_.start(CONFIG->nacos_addr_, this);    
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    //quote.print();
    if( !CONFIG->replay_mode_ && CONFIG->dump_binary_ ) {
        quote_dumper_.on_snap(exchange, symbol, quote);
    }
    
    if( CONFIG->publish_data_ ) {
        quote_mixer2_.on_snap(exchange, symbol, quote);
    }
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    //quote.print();
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

void StreamEngine::on_trade(const TExchange& exchange, const TSymbol& symbol, const Trade& trade)
{
    quote_mixer2_.on_trade(exchange, symbol, trade);
}

void StreamEngine::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init)
{
    for( const auto& v : kline ) {        
        _log_and_print("get %s-%s kline %d index=%lu open=%s high=%s low=%s close=%s volume=%s", exchange.c_str(), symbol.c_str(), resolution, 
            v.index,
            v.px_open.get_str_value().c_str(),
            v.px_high.get_str_value().c_str(),
            v.px_low.get_str_value().c_str(),
            v.px_close.get_str_value().c_str(),
            v.volume.get_str_value().c_str()
        );
    }
    if( exchange != "" )
        kline_mixer_.on_kline(exchange, symbol, resolution, kline);
    kline_hubber_.on_kline(exchange, symbol, resolution, kline, is_init);
}

void StreamEngine::signal_handler(int signum)
{
    UT_LOG_INFO(CONFIG->logger_, "StreamEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    // 退出
    exit(0);
} 

unordered_map<TExchange, SymbolFee> to_fee_config(const unordered_map<TExchange, SNacosConfigByExchange>& exchanges) 
{    
    unordered_map<TExchange, SymbolFee> fees;
    for( const auto& v : exchanges ) {
        SymbolFee tmp;
        tmp.fee_type = v.second.fee_type;
        tmp.maker_fee = v.second.fee_maker;
        tmp.taker_fee = v.second.fee_taker;
        fees[v.first] = tmp;
    }
    return fees;
}

template<typename T>
bool map_equal(const unordered_map<TExchange, T>& m1, const unordered_map<TExchange, T>& m2) {
    for( const auto& v : m1 ) {
        auto iter = m2.find(v.first);
        if( iter == m2.end() )
            return false;
        const T& f1 = v.second;
        const T& f2 = iter->second;
        if( memcmp(&f1, &f2, sizeof(T)) != 0 )
            return false;
    }
    for( const auto& v : m2 ) {
        if( m1.find(v.first) == m1.end() )
            return false;
    }
    return true;
}

unordered_map<TExchange, RedisQuote::ExchangeConfig> to_redis_config(const unordered_map<TExchange, SNacosConfigByExchange>& configs)
{
    unordered_map<TExchange, RedisQuote::ExchangeConfig> ret;
    for( const auto& v : configs ) {
        ret[v.first].precise = v.second.precise;
        ret[v.first].vprecise = v.second.vprecise;
    }
    return ret;
}

void StreamEngine::on_config_channged(const NacosString& configInfo)
{
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

    // string -> 结构化数据
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
        cfg.vprecise = symbol_cfgs["vprecise"].get<int>();
        cfg.depth = symbol_cfgs["depth"].get<unsigned int>();
        cfg.frequency = symbol_cfgs["frequency"].get<float>();
        cfg.mix_depth = symbol_cfgs["mix_depth"].get<unsigned int>();
        cfg.mix_frequecy = symbol_cfgs["mix_frequency"].get<float>();
        for( auto iter2 = symbol_cfgs["exchanges"].begin() ; iter2 != symbol_cfgs["exchanges"].end() ; ++iter2 )
        {
            const TExchange& exchange = iter2.key();
            const njson& exchange_cfgs = iter2.value();
            SNacosConfigByExchange exchange_cfg;
            exchange_cfg.precise = exchange_cfgs["precise"].get<int>();
            exchange_cfg.precise = exchange_cfgs["vprecise"].get<int>();
            exchange_cfg.fee_type = exchange_cfgs["fee_type"].get<int>();
            exchange_cfg.fee_maker = exchange_cfgs["fee_maker"].get<float>();
            exchange_cfg.fee_taker = exchange_cfgs["fee_taker"].get<float>();
            cfg.exchanges[exchange] = exchange_cfg;
        } 
        symbols[symbol] = cfg;
    }
    
    // quote_source_ 涉及交易所+交易币种+原始行情精度，原始行情更新频率
    // quote_mixer2_ 涉及交易币种的精度，深度，频率，各交易所手续费
    
    for( const auto& v : symbols )
    {
        const TSymbol& symbol = v.first;
        const SNacosConfig& config = v.second;

        // 新增品种
        if( symbols_.find(symbol) == symbols_.end() ) 
        {
            quote_mixer2_.set_compute_params(symbol, config.precise, config.mix_depth, to_fee_config(config.exchanges));
            quote_mixer2_.set_publish_params(symbol, config.mix_frequecy);
            kline_mixer_.set_symbol(symbol, config.get_exchanges());
            quote_source_.set_config(symbol, config.frequency, to_redis_config(config.exchanges));
        }
        // 已有的品种
        else 
        { 
            const SNacosConfig& last_config = symbols_[symbol];
            // 源头配置变更
            if( last_config.frequency != config.frequency || !map_equal(to_redis_config(last_config.exchanges), to_redis_config(config.exchanges))) {
                quote_source_.set_config(symbol, config.frequency, to_redis_config(config.exchanges));
            }
            // 交易所数量变更
            if( last_config.get_exchanges() != config.get_exchanges() ) {
                kline_mixer_.set_symbol(symbol, config.get_exchanges());
            }
            // 精度，各交易所手续费 
            if( last_config.precise != config.precise || !map_equal(to_fee_config(last_config.exchanges), to_fee_config(config.exchanges)) || last_config.mix_depth != config.mix_depth) {
                quote_mixer2_.set_compute_params(symbol, config.precise, config.mix_depth, to_fee_config(config.exchanges));
            }
            // 频率
            if( last_config.mix_frequecy != config.mix_frequecy ) {
                quote_mixer2_.set_publish_params(symbol, config.mix_frequecy);
            }
        }
    }


    // 删除品种
    for( const auto& v : symbols_ ) {
        if( symbols.find(v.first) == symbols.end() ) {
            quote_source_.set_config(v.first, 0, unordered_map<TExchange, RedisQuote::ExchangeConfig>());
        }
    }

    // 赋值
    symbols_ = symbols;

    CONFIG->set_config(configInfo);
}