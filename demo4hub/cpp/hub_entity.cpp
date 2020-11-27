#include "hub_config.h"
#include "hub_entity.h"
#include "hub_interface.h"

// config file relative path
const char* config_file = "config.json";

HubEntity::HubEntity()
{
    // init configuration
    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file);
}

HubEntity::~HubEntity()
{

}

int HubEntity::start()
{
    quote_updater_.start(CONFIG->grpc_server_addr_, this);
    return 0;
}

int HubEntity::stop()
{
    return 0;
}

int HubEntity::get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    return 0;
}

void HubEntity::on_snap(const SEData& quote)
{
    SDepthData quote_depth;
    quote_depth.seqno = quote.seq_no();
    quote_depth.tick = quote.time();
    for( int i = 0 ; i < quote.asks_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = quote.asks(i);
        quote_depth.asks[i].price = SDecimal(depth.price());
        quote_depth.asks[i].volume = depth.volume();
        quote_depth.ask_length = i+1;
    }
    for( int i = 0 ; i < quote.bids_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = quote.bids(i);
        quote_depth.bids[i].price = SDecimal(depth.price());
        quote_depth.bids[i].volume = depth.volume();
        quote_depth.bid_length = i+1;
    }

    callback_->on_depth("", quote.symbol().c_str(), quote_depth);
}
