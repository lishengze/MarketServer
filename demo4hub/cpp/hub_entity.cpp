#include "hub_config.h"
#include "hub_entity.h"
#include "hub_interface.h"
#include <iostream>
using std::endl;

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
    cout << "HubEntity::start "<< endl;
    quote_updater_.start(CONFIG->risk_controller_addr_, this);
    kline_updater_.start(CONFIG->stream_engine_addr_, this);
    return 0;
}

int HubEntity::stop()
{
    return 0;
}

int HubEntity::get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    RequestKline(CONFIG->stream_engine_addr_, exchange, symbol, resolution, start_time, end_time, klines);
    return 0;
}

void HubEntity::on_snap(const SEData& quote)
{
    SDepthData quote_depth;
    quote_depth.seqno = quote.seq_no();
    quote_depth.tick = quote.time();
    for( int i = 0 ; i < quote.asks_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = quote.asks(i);
        Decimal_to_SDecimal(quote_depth.asks[i].price, depth.price());
        Decimal_to_SDecimal(quote_depth.asks[i].volume, depth.volume());
        quote_depth.ask_length = i+1;
    }
    for( int i = 0 ; i < quote.bids_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = quote.bids(i);
        Decimal_to_SDecimal(quote_depth.asks[i].price, depth.price());
        Decimal_to_SDecimal(quote_depth.asks[i].volume, depth.volume());
        quote_depth.bid_length = i+1;
    }
    quote_depth.symbol = quote.symbol();
    quote_depth.exchange = quote.exchange();

    //cout << "HubEntity::on_snap " << quote_depth.symbol << " " << quote_depth.ask_length << " " << quote_depth.bid_length << endl;

    // for (int i =0; i < 10; ++i)
    // {
    //     cout << quote_depth.asks[i].price.get_value() << ", " << quote_depth.bids[i].price.get_value() << endl;
    // }
    callback_->on_depth("", quote.symbol().c_str(), quote_depth);
}

void HubEntity::on_kline(const SEKlineData& quote)
{
    vector<KlineData> klines;
    for( int i = 0 ; i < quote.klines_size() ; i ++ ) {
        const SEKline& kline = quote.klines(i);
        KlineData _kline;
        _kline.index = kline.index();
        Decimal_to_SDecimal(_kline.px_open, kline.open());
        Decimal_to_SDecimal(_kline.px_high, kline.high());
        Decimal_to_SDecimal(_kline.px_low, kline.low());
        Decimal_to_SDecimal(_kline.px_close, kline.close());
        Decimal_to_SDecimal(_kline.volume, kline.volume());
        klines.push_back(_kline);
    }
    callback_->on_kline(quote.exchange().c_str(), quote.symbol().c_str(), quote.resolution(), klines);
}
