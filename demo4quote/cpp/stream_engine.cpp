#include "stream_engine.h"
#include "stream_engine_config.h"


// config file relative path
const char* config_file = "config.json";

StreamEngine::StreamEngine(){
    
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);

    redis_quote_ = new RedisQuote();
    redis_quote_->init(CONFIG->quote_redis_host_, CONFIG->quote_redis_port_, CONFIG->quote_redis_password_, CONFIG->logger_);
    redis_quote_->set_engine(this);

    quote_mixer_ = new QuoteMixer();

    quote_dumper_ = new QuoteDumper();
}

StreamEngine::~StreamEngine(){
}

void StreamEngine::start() {
    //redis_quote_->start();
    quote_mixer_->publish_fake();
}

void StreamEngine::on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote){
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    markets_[exchange][symbol] = quote;
    if( CONFIG->dump_binary_only_ )
        quote_dumper_->on_mix_snap(exchange, symbol, quote);
    else
        quote_mixer_->on_mix_snap(exchange, symbol, quote);
};

void StreamEngine::on_update(const string& exchange, const string& symbol, const SDepthQuote& quote){  
    std::unique_lock<std::mutex> inner_lock{ mutex_markets_ };
    SDepthQuote lastQuote;
    if( !_get_quote(exchange, symbol, lastQuote) )
        return;
    // filter by SequenceNo
    if( quote.SequenceNo < lastQuote.SequenceNo )
        return;
    if( CONFIG->dump_binary_only_ )
        quote_dumper_->on_mix_snap(exchange, symbol, quote);
    else
        quote_mixer_->on_mix_update(exchange, symbol, quote);
};

bool StreamEngine::_get_quote(const string& exchange, const string& symbol, SDepthQuote& quote) const {
    auto iter = markets_.find(exchange);
    if( iter == markets_.end() )
        return false;
    const TMarketQuote& marketQuote = iter->second;
    auto iter2 = marketQuote.find(symbol);
    if( iter2 == marketQuote.end() )
        return false;
    quote = iter2->second;
    return true;
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