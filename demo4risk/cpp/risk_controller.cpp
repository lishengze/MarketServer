#include "risk_controller.h"
#include "risk_controller_config.h"
#include "grpc_server.h"


// config file relative path
const char* config_file = "config.json";

void quotedata_to_innerquote(const QuoteData& src, SInnerQuote& dst) {
    strcpy(dst.symbol, src.symbol().c_str());
    dst.seq_no = src.msg_seq();
    //dst.time = src.time();
    //dst.time_arrive = src.time_arrive();
    // 卖盘
    for( int i = 0 ; i < src.ask_depth_size() && i < MAX_DEPTH_LENGTH ; ++i ) {
        const DepthLevel& srcDepth = src.ask_depth(i);
        dst.asks[i].price.Value = srcDepth.price().value();
        dst.asks[i].price.Base = srcDepth.price().base();
        for( int j = 0 ; j < srcDepth.data_size() && j < MAX_EXCHANGE_LENGTH ; ++j ) {
            const DepthVolume& srcDepthVolume = srcDepth.data(j);
            strcpy(dst.asks[i].exchanges[j].name, srcDepthVolume.exchange().c_str());
            dst.asks[i].exchanges[j].volume = srcDepthVolume.volume();
            dst.asks[i].exchange_length = j+1;
        }
        dst.ask_length = i+1;
    }
    // 买盘
    for( int i = 0 ; i < src.bid_depth_size() && i < MAX_DEPTH_LENGTH ; ++i ) {
        const DepthLevel& srcDepth = src.bid_depth(i);
        dst.bids[i].price.Value = srcDepth.price().value();
        dst.bids[i].price.Base = srcDepth.price().base();
        for( int j = 0 ; j < srcDepth.data_size() && j < MAX_EXCHANGE_LENGTH ; ++j ) {
            const DepthVolume& srcDepthVolume = srcDepth.data(j);
            strcpy(dst.bids[i].exchanges[j].name, srcDepthVolume.exchange().c_str());
            dst.bids[i].exchanges[j].volume = srcDepthVolume.volume();
            dst.bids[i].exchange_length = j+1;
        }
        dst.bid_length = i+1;
    }
}

RiskController::RiskController(){
    
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    utrade::pandora::Singleton<ServerImpl>::Instance();
}

RiskController::~RiskController(){
}

void RiskController::start() {
    quote_updater_.start(CONFIG->grpc_quote_addr_, this);
    configuration_updater_.start(this);
    account_updater_.start(this);

    // start grpc server
    PUBLISHER->run_in_thread(CONFIG->grpc_publish_addr_, &datacenter_);
}

void RiskController::signal_handler(int signum)
{
    //UT_LOG_INFO(GALAXY_LOGGER, "KernelEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    //KERNELENGINE->release();
    // 退出
    exit(0);
}

void RiskController::on_snap(const QuoteData& quote)
{
    // QuoteData to SInnerQuote
    SInnerQuote raw;
    quotedata_to_innerquote(quote, raw);
    datacenter_.add_quote(raw);
}

void RiskController::on_configuration_update(const QuoteConfiguration& config)
{
    datacenter_.change_configuration(config);
}

void RiskController::on_account_update(const AccountInfo& account)
{
    datacenter_.change_account(account);
}

void RiskController::on_order_update(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
    datacenter_.change_orders(symbol, order, asks, bids);
}
