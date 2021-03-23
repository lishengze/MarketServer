#include "risk_controller.h"
#include "risk_controller_config.h"
#include "grpc_server.h"
#include "base/cpp/grpc_client.h"
#include "updater_configuration.h"

// config file relative path
const char* config_file = "config.json";

void quotedata_to_innerquote(const SEData& src, SInnerQuote& dst) {
    dst.exchange = src.exchange();
    dst.symbol = src.symbol();
    dst.precise = src.price_precise();
    dst.vprecise = src.volume_precise();
    dst.time_origin = src.time();
    dst.time_arrive_at_streamengine = src.time_arrive();
    dst.time_produced_by_streamengine = src.time_produced_by_streamengine();
    dst.time_arrive = get_miliseconds();
    
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

RiskControllerServer::RiskControllerServer()
{
    // load config here...
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
    
    // init server
    utrade::pandora::Singleton<ServerEndpoint>::Instance();
    server_endpoint_.set_cacher(&datacenter_); // 必须在init之前
    server_endpoint_.init(CONFIG->grpc_publish_addr_);
    datacenter_.register_callback(&server_endpoint_);
}

RiskControllerServer::~RiskControllerServer(){
}

void RiskControllerServer::start() {
    quote_updater_.start(CONFIG->grpc_quote_addr_, this);
    RISK_CONFIG->set_callback(this);
    RISK_CONFIG->start(CONFIG->nacos_addr_, CONFIG->nacos_namespace_);
    account_updater_.start(CONFIG->grpc_account_addr_, this);

    // start grpc server
    server_endpoint_.start();
}

void RiskControllerServer::signal_handler(int signum)
{
    //UT_LOG_INFO(GALAXY_LOGGER, "KernelEngine::signal_handler " << signum);
    //signal_sys = signum;
    // 释放资源
    //KERNELENGINE->release();
    // 退出
    exit(0);
}

void RiskControllerServer::on_snap(const SEData& quote)
{
    // QuoteData to SInnerQuote
    SInnerQuote raw;
    quotedata_to_innerquote(quote, raw);
    //std::cout << "update symbol " << quote.symbol() << " " << raw.asks.size() << "/" << raw.bids.size() << std::endl;
    datacenter_.add_quote(raw);
}

void RiskControllerServer::on_configuration_update(const map<TSymbol, QuoteConfiguration>& config)
{
    datacenter_.change_configuration(config);
}

void RiskControllerServer::on_account_update(const AccountInfo& account)
{
    datacenter_.change_account(account);
}

void RiskControllerServer::on_order_update(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
    datacenter_.change_orders(symbol, order, asks, bids);
}
