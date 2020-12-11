#include "risk_controller.h"
#include "risk_controller_config.h"
#include "grpc_server.h"


// config file relative path
const char* config_file = "config.json";

void quotedata_to_innerquote(const SEData& src, SInnerQuote& dst) {
    dst.symbol = src.symbol();
    //vassign(dst.seq_no, src.msg_seq());
    // 卖盘
    for( int i = 0 ; i < src.asks_size() ; ++i ) {
        const SEDepth& src_depth = src.asks(i);
        SDecimal price = SDecimal::parse_by_raw(src_depth.price().base(), src_depth.price().prec());
        SInnerDepth depth;
        for( auto v : src_depth.data() ) {
            depth.exchanges[v.first] = SDecimal::parse_by_raw(v.second.base(), v.second.prec());
        }
        dst.asks[price] = depth;
    }
    // 买盘
    for( int i = 0 ; i < src.bids_size() ; ++i ) {
        const SEDepth& src_depth = src.bids(i);
        SDecimal price = SDecimal::parse_by_raw(src_depth.price().base(), src_depth.price().prec());
        SInnerDepth depth;
        for( auto v : src_depth.data() ) {
            depth.exchanges[v.first] = SDecimal::parse_by_raw(v.second.base(), v.second.prec());
        }
        dst.bids[price] = depth;
    }
}

RiskController::RiskController(){
    
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    utrade::pandora::Singleton<ServerEndpoint>::Instance();
    PUBLISHER->init(CONFIG->grpc_publish_addr_);
}

RiskController::~RiskController(){
}

void RiskController::start() {
    quote_updater_.start(CONFIG->grpc_quote_addr_, this);
    configuration_updater_.start(this);
    account_updater_.start(CONFIG->grpc_account_addr_, this);

    // start grpc server
    PUBLISHER->run_in_thread();
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
