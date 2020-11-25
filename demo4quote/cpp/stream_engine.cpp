#include "stream_engine.h"
#include "stream_engine_config.h"


// config file relative path
const char* config_file = "config.json";

StreamEngine::StreamEngine(){
    
    // init configuration
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    // init grpc server
    utrade::pandora::Singleton<GrpcServer>::Instance();
    PUBLISHER->init(CONFIG->grpc_publish_addr_);
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() {
    //nacos_client_.start();
    
    if( !CONFIG->replay_mode_ ) {
        // start redis
        quote_source_.set_engine(this);
        RedisParams params;
        params.host = CONFIG->quote_redis_host_;
        params.port = CONFIG->quote_redis_port_;
        params.password = CONFIG->quote_redis_password_;
        quote_source_.start(params, CONFIG->logger_);

        // 启动录数据线程
        quote_dumper_.start();
    } else {
        quote_replay_.set_engine(this);
        quote_replay_.start(CONFIG->replay_ratio_);
    }

    // start grpc server
    PUBLISHER->run_in_thread();
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    if( !CONFIG->replay_mode_ && CONFIG->dump_binary_ ) {
        quote_dumper_.on_snap(exchange, symbol, quote);
    }
    
    if( CONFIG->publish_data_ || !CONFIG->is_forbidden_exchage(exchange) ) {
        quote_mixer2_.on_snap(exchange, symbol, quote);
        quote_single_.on_snap(exchange, symbol, quote);
    }
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    if( !CONFIG->replay_mode_ && CONFIG->dump_binary_ ) {
        quote_dumper_.on_update(exchange, symbol, quote);
    }

    if( CONFIG->publish_data_ || !CONFIG->is_forbidden_exchage(exchange) ) {
        quote_mixer2_.on_update(exchange, symbol, quote);
        quote_single_.on_update(exchange, symbol, quote);
    }
};

void StreamEngine::on_connected() 
{
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
            const string& symbol = *iterSymbol;
            const string& exchange = *iterExchange;
            quote_source_.subscribe("UPDATEx|" + symbol + "." + exchange);            
        }
    }
}

void StreamEngine::on_nodata_exchange(const TExchange& exchange) 
{
    quote_mixer2_.clear_exchange(exchange);
    quote_single_.clear_exchange(exchange);
}

void StreamEngine::on_precise_changed(const TSymbol& symbol, int precise) 
{
    quote_mixer2_.change_precise(symbol, precise);
    quote_source_.change_precise(symbol, precise);
}

void StreamEngine::signal_handler(int signum)
{
    UT_LOG_INFO(CONFIG->logger_, "StreamEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    // 退出
    exit(0);
}
