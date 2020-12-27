#include "hub_config.h"
#include "hub_entity.h"
#include "hub_interface.h"
#include "quark/cxx/assign.h"
#include <iostream>
using std::endl;

// config file relative path
const char* config_file = "config.json";

void SETradeToTrade(const SETrade& src, Trade& dst)
{    
    assign(dst.symbol, src.symbol());
    assign(dst.exchange, src.exchange());
    dst.time = src.time();
    Decimal_to_SDecimal(dst.price, src.price());
    Decimal_to_SDecimal(dst.volume, src.volume());
}

void SEKlineToKline(const SEKline& src, KlineData& dst)
{    
    dst.index = src.index();
    Decimal_to_SDecimal(dst.px_open, src.open());
    Decimal_to_SDecimal(dst.px_high, src.high());
    Decimal_to_SDecimal(dst.px_low, src.low());
    Decimal_to_SDecimal(dst.px_close, src.close());
    Decimal_to_SDecimal(dst.volume, src.volume());
}

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
    trade_updater_.start(CONFIG->stream_engine_addr_, this);
    return 0;
}

int HubEntity::stop()
{
    return 0;
}

int HubEntity::get_kline(const char* exchange, const char* symbol, type_resolution resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    vector<SEKline> raw;
    RequestKline(CONFIG->stream_engine_addr_, exchange, symbol, resolution, start_time, end_time, raw);
    klines.clear();
    klines.reserve(raw.size());
    for( const auto& v : raw ) {
        KlineData tmp;
        SEKlineToKline(v, tmp);
        klines.push_back(tmp);
    }
    return 0;
    return 0;
}

int HubEntity::get_lasttrades(vector<Trade>& trades)
{
    vector<SETrade> raw;
    RequestLastTrades(CONFIG->stream_engine_addr_, raw);
    trades.clear();
    trades.reserve(raw.size());
    for( const auto& v : raw ) {
        Trade tmp;
        SETradeToTrade(v, tmp);
        trades.push_back(tmp);
    }
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

    assign(quote_depth.symbol, quote.symbol());
    assign(quote_depth.exchange, quote.exchange());

    //cout << "HubEntity::on_snap " << quote_depth.symbol << " " << quote_depth.ask_length << " " << quote_depth.bid_length << endl;

    // for (int i =0; i < 10; ++i)
    // {
    //     cout << quote_depth.asks[i].price.get_value() << ", " << quote_depth.bids[i].price.get_value() << endl;
    // }
    callback_->on_depth("", quote.symbol().c_str(), quote_depth);
}

void HubEntity::on_trade(const SETrade& trade)
{
    Trade _trade;
    SETradeToTrade(trade, _trade);
    callback_->on_trade(trade.exchange().c_str(), trade.symbol().c_str(), _trade);
}

void HubEntity::on_kline(const SEKlineData& quote)
{
    vector<KlineData> klines;
    for( int i = 0 ; i < quote.klines_size() ; i ++ ) {
        const SEKline& kline = quote.klines(i);
        KlineData _kline;
        _kline.index = kline.index();
        assign(_kline.symbol, quote.symbol());
        assign(_kline.exchange, quote.exchange());
        SEKlineToKline(kline, _kline);
        klines.push_back(_kline);
    }
    callback_->on_kline(quote.exchange().c_str(), quote.symbol().c_str(), quote.resolution(), klines);
}
