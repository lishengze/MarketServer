#include "risk_controller.h"
#include "risk_controller_config.h"
#include "grpc_server.h"


// config file relative path
const char* config_file = "config.json";

void quotedata_to_innerquote(const SEData& src, SInnerQuote& dst) {
    vassign(dst.symbol, MAX_SYMBOLNAME_LENGTH, src.symbol());
    //vassign(dst.seq_no, src.msg_seq());
    // 卖盘
    for( int i = 0 ; i < src.ask_depths_size() && i < MAX_DEPTH_LENGTH ; ++i ) {
        const SEDepth& src_depth = src.ask_depths(i);
        SInnerDepth& dst_depth = dst.asks[i];
        dst_depth.price.from(src_depth.price());
        int count = 0;
        for( auto v : src_depth.data() ) {
            vassign(dst_depth.exchanges[count].name, MAX_EXCHANGENAME_LENGTH, v.first);
            vassign(dst_depth.exchanges[count].volume, v.second);
            count++;
            if( count >= MAX_EXCHANGE_LENGTH )
                break;
        }
        dst_depth.exchange_length = count;
        dst.ask_length = i+1; 
    }
    // 买盘
    for( int i = 0 ; i < src.bid_depths_size() && i < MAX_DEPTH_LENGTH ; ++i ) {
        const SEDepth& src_depth = src.bid_depths(i);
        SInnerDepth& dst_depth = dst.bids[i];
        dst_depth.price.from(src_depth.price());
        int count = 0;
        for( auto v : src_depth.data() ) {
            vassign(dst_depth.exchanges[count].name, MAX_EXCHANGENAME_LENGTH, v.first);
            vassign(dst_depth.exchanges[count].volume, v.second);
            count++;
            if( count >= MAX_EXCHANGE_LENGTH )
                break;
        }
        dst_depth.exchange_length = count;
        dst.bid_length = i+1; 
    }
}

RiskController::RiskController(){
    
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    utrade::pandora::Singleton<GrpcServer>::Instance();
}

RiskController::~RiskController(){
}

void RiskController::start() {
    quote_updater_.start(CONFIG->grpc_quote_addr_, this);
    configuration_updater_.start(this);
    account_updater_.start(this);

    // start grpc server
    PUBLISHER->run_in_thread(CONFIG->grpc_publish_addr_);
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

void RiskController::on_snap(const SEData& quote)
{
    // QuoteData to SInnerQuote
    SInnerQuote raw;
    quotedata_to_innerquote(quote, raw);
    //std::cout << "update symbol " << quote.symbol() << " " << raw.ask_length << "/" << raw.bid_length << std::endl;
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
