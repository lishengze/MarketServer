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
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() {
    // start redis
    quote_source_.set_engine(this);
    RedisParams params;
    params.host = CONFIG->quote_redis_host_;
    params.port = CONFIG->quote_redis_port_;
    params.password = CONFIG->quote_redis_password_;
    quote_source_.start(params, CONFIG->logger_);

    // start grpc server
    PUBLISHER->run_in_thread(CONFIG->grpc_publish_addr_);
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    if( CONFIG->dump_binary_ ) {
        quote_dumper_.on_snap(exchange, symbol, quote);
    }
    
    if( CONFIG->publish_data_ ) {
        if( CONFIG->mixer_ver_ == 1 ) {
            quote_mixer_.on_snap(exchange, symbol, quote);
        } else if( CONFIG->mixer_ver_ == 2 ) {
            quote_mixer2_.on_snap(exchange, symbol, quote);
        } else {
            UT_LOG_ERROR(CONFIG->logger_, "unknown mixer_ver " << CONFIG->mixer_ver_);
        }
        quote_single_.on_snap(exchange, symbol, quote);
    }
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    if( CONFIG->dump_binary_ ) {
        quote_dumper_.on_mix_update(exchange, symbol, quote);
    }

    if( CONFIG->publish_data_ ) {
        if( CONFIG->mixer_ver_ == 1 ) {
            quote_mixer_.on_update(exchange, symbol, quote);
        } else if( CONFIG->mixer_ver_ == 2 ) {
            quote_mixer2_.on_update(exchange, symbol, quote);
        } else {
            UT_LOG_ERROR(CONFIG->logger_, "unknown mixer_ver " << CONFIG->mixer_ver_);
        }
        quote_single_.on_update(exchange, symbol, quote);
    }
};

void StreamEngine::on_connected() {    
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
            const string& symbol = *iterSymbol;
            const string& exchange = *iterExchange;
            quote_source_.subscribe("UPDATEx|" + symbol + "." + exchange);            
        }
    }
};

void StreamEngine::signal_handler(int signum)
{
    UT_LOG_INFO(CONFIG->logger_, "StreamEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    // 退出
    exit(0);
}