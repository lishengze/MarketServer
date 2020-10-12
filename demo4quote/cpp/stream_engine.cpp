#include "stream_engine.h"
#include "stream_engine_config.h"


// config file relative path
const char* config_file = "config.json";

StreamEngine::StreamEngine(){
    
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    utrade::pandora::Singleton<ServerImpl>::Instance();
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() {
    // start redis
    redis_quote_.set_engine(this);
    redis_quote_.start(CONFIG->quote_redis_host_, CONFIG->quote_redis_port_, CONFIG->quote_redis_password_, CONFIG->logger_);

    // start grpc server
    PUBLISHER->run_in_thread(CONFIG->grpc_push_addr_);
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    if( CONFIG->dump_binary_only_ )
        quote_dumper_.on_mix_snap(exchange, symbol, quote);
    
    quote_mixer_.on_snap(exchange, symbol, quote);

    PUBLISHER->on_snap(exchange, symbol, quote);
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    if( CONFIG->dump_binary_only_ )
        quote_dumper_.on_mix_update(exchange, symbol, quote);

    quote_mixer_.on_update(exchange, symbol, quote);
    
    PUBLISHER->on_update(exchange, symbol, quote);
};

void StreamEngine::on_connected() {    
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
            const string& symbol = *iterSymbol;
            const string& exchange = *iterExchange;
            redis_quote_.subscribe("UPDATEx|" + symbol + "." + exchange);            
        }
    }
};

void StreamEngine::signal_handler(int signum)
{
    //UT_LOG_INFO(GALAXY_LOGGER, "KernelEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    //KERNELENGINE->release();
    // 退出
    exit(0);
}