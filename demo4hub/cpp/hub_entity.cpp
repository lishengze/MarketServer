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
    //trade_updater_.start(CONFIG->stream_engine_addr_, this);
    depth_updater_.start(CONFIG->stream_engine_addr_, this);
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

void SEData_to_SDepthData(const SEData& src, SDepthData& dst)
{
    dst.seqno = src.seq_no();
    dst.tick = src.time();
    dst.tick1 = src.time_arrive();
    dst.tick2 = src.time_produced_by_streamengine();
    dst.tick3 = src.time_produced_by_riskcontrol();
    dst.precise = src.price_precise();
    dst.vprecise = src.volume_precise();
    dst.aprecise = src.amount_precise();
    for( int i = 0 ; i < src.asks_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = src.asks(i);
        Decimal_to_SDecimal(dst.asks[i].price, depth.price());
        Decimal_to_SDecimal(dst.asks[i].volume, depth.volume());
        dst.ask_length = i+1;
    }
    for( int i = 0 ; i < src.bids_size() && i < DEPCH_LEVEL_COUNT; i ++ ) {
        const SEDepth& depth = src.bids(i);
        Decimal_to_SDecimal(dst.bids[i].price, depth.price());
        Decimal_to_SDecimal(dst.bids[i].volume, depth.volume());
        dst.bid_length = i+1;
    }

    assign(dst.symbol, src.symbol());
    assign(dst.exchange, src.exchange());
}

void HubEntity::on_snap(const SEData& quote)
{
    SDepthData quote_depth;
    SEData_to_SDepthData(quote, quote_depth);
    callback_->on_depth(quote.exchange().c_str(), quote.symbol().c_str(), quote_depth);
}

void HubEntity::on_raw_depth(const SEData& quote)
{
    SDepthData quote_depth;
    SEData_to_SDepthData(quote, quote_depth);
    callback_->on_raw_depth(quote.exchange().c_str(), quote.symbol().c_str(), quote_depth);
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
