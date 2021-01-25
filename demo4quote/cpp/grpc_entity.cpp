#include "stream_engine_config.h"
#include "grpc_entity.h"
#include "quote_mixer2.h"

#define ONE_ROUND_MESSAGE_NUMBRE 999

template<class T>
void copy_protobuf_object(const T* src, T* dst) {
    string tmp;
    if( !src->SerializeToString(&tmp) )
        return;
    dst->ParseFromString(tmp);
}

void trade_to_trade(const TradeWithDecimal* src, TradeWithDecimal* dst) {
    dst->set_exchange(src->exchange());
    dst->set_symbol(src->symbol());
    dst->set_time(src->time());
    set_decimal(dst->mutable_price(), src->price());
    set_decimal(dst->mutable_volume(), src->volume());
}

void quote_to_quote(const MarketStreamDataWithDecimal* src, MarketStreamDataWithDecimal* dst) {
    dst->set_exchange(src->exchange());
    dst->set_symbol(src->symbol());
    dst->set_seq_no(src->seq_no());
    dst->set_is_snap(src->is_snap());
    dst->set_time(src->time());
    dst->set_price_precise(src->price_precise());
    dst->set_volume_precise(src->volume_precise());

    // 卖盘
    for( int i = 0 ; i < src->asks_size() ; ++i ) {
        const DepthWithDecimal& src_depth = src->asks(i);
        DepthWithDecimal* dst_depth = dst->add_asks();
        set_decimal(dst_depth->mutable_price(), src_depth.price());
        set_decimal(dst_depth->mutable_volume(), src_depth.volume());
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bids_size() ; ++i ) {
        const DepthWithDecimal& src_depth = src->bids(i);
        DepthWithDecimal* dst_depth = dst->add_bids();
        set_decimal(dst_depth->mutable_price(), src_depth.price());
        set_decimal(dst_depth->mutable_volume(), src_depth.volume());
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
};

void kline_to_pbkline(const KlineData& src, Kline* dst)
{
    dst->set_index(src.index);
    set_decimal(dst->mutable_open(), src.px_open);
    set_decimal(dst->mutable_high(), src.px_high);
    set_decimal(dst->mutable_low(), src.px_low);
    set_decimal(dst->mutable_close(), src.px_close);
    set_decimal(dst->mutable_volume(), src.volume);
}

//////////////////////////////////////////////////
SubscribeSingleQuoteEntity::SubscribeSingleQuoteEntity(void* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeSingleQuoteEntity::register_call()
{
    std::cout << "register SubscribeSingleQuoteEntity" << std::endl;
    service_->RequestSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeSingleQuoteEntity::on_init() 
{
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    if( cacher_->get_lastsnap(snaps) )
    {            
        for( const auto& v : snaps ) {
            if( _is_filtered(v->exchange(), v->symbol()) )
                continue;
            datas_.enqueue(v);
        }
    }
}

/*
void compare_pb_json2(const MarketStreamDataWithDecimal& quote)
{
    std::cout << "---------------------" << std::endl;

    string a;
    quote.SerializeToString(&a);
    long long tbegin, tend;
    
    tbegin = get_miliseconds();
    for( int i = 0 ; i < 1000 ; ++i ) {
        string tmp;
        quote.SerializeToString(&tmp);
    }
    tend = get_miliseconds();
    std::cout << "serialize pb use:" << (tend - tbegin) << std::endl;
    
    tbegin = get_miliseconds();
    for( int i = 0 ; i < 1000 ; ++i ) {
        MarketStreamDataWithDecimal tmp;
        if( !tmp.ParseFromString(a) )
            std::cout << "parse fail" << std::endl;
    }
    tend = get_miliseconds();
    std::cout << "parse pb use:" << (tend - tbegin) << std::endl;

    std::cout << "pb-size:" << a.length() << std::endl;
    // 转为json
    njson obj;
    obj["symbol"] = quote.symbol();
    obj["exchange"] = quote.exchange();
    obj["is_snap"] = quote.is_snap();
    for( int i = 0 ; i < quote.asks_size() ; ++i ) {
        obj["ask_depth"][quote.asks(i).price()] = quote.asks(i).volume();
    }
    for( int i = 0 ; i < quote.bids_size() ; ++i ) {
        obj["bid_depth"][quote.bids(i).price()] = quote.bids(i).volume();
    }

    a = obj.dump();
    std::cout << "json-size:" << a.length() << std::endl;

    tbegin = get_miliseconds();
    for( int i = 0 ; i < 1000 ; ++i ) {
        obj.dump();
    }
    tend = get_miliseconds();
    std::cout << "serialize json use:" << (tend - tbegin) << std::endl;
    
    tbegin = get_miliseconds();
    for( int i = 0 ; i < 1000 ; ++i ) {
        njson js = njson::parse(a);
    }
    tend = get_miliseconds();
    std::cout << "parse json use:" << (tend - tbegin) << std::endl;
    std::cout << "---------------------" << std::endl;
}
*/
bool SubscribeSingleQuoteEntity::process()
{
    MultiMarketStreamDataWithDecimal reply;
    type_tick now = get_miliseconds();

    StreamDataPtr ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)ptrs[i].get(), quote);
        quote->set_time_produced_by_streamengine(now);
    }    

    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    } 
}

bool SubscribeSingleQuoteEntity::_is_filtered(const TExchange& exchange, const TSymbol& symbol)
{
    if( string(request_.exchange()) != "" && string(request_.exchange()) != exchange ) {
        return true;
    }
    if( string(request_.symbol()) != "" && string(request_.symbol()) != symbol ) {
        return true;
    }
    return false;
}

void SubscribeSingleQuoteEntity::add_data(StreamDataPtr data) 
{
    if( _is_filtered(data->exchange(), data->symbol()) )
        return;

    datas_.enqueue(data);
}

//////////////////////////////////////////////////
SubscribeMixQuoteEntity::SubscribeMixQuoteEntity(void* service, IMixerCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeMixQuoteEntity::register_call()
{
    std::cout << "register SubscribeMixQuoteEntity" << std::endl;
    service_->RequestSubscribeMixQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeMixQuoteEntity::on_init() 
{
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    cacher_->get_lastsnaps(snaps);
    tfm::printfln("get_lastsnaps %u items", snaps.size());

    for( const auto& v : snaps ){
        datas_.enqueue(v);
    }
}

bool SubscribeMixQuoteEntity::process()
{    
    MultiMarketStreamDataWithDecimal reply;
    type_tick now = get_miliseconds();

    StreamDataPtr ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)ptrs[i].get(), quote);
        quote->set_time_produced_by_streamengine(now);
    }    

    // 执行发送
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void SubscribeMixQuoteEntity::add_data(StreamDataPtr data) 
{
    datas_.enqueue(data);
}

//////////////////////////////////////////////////
GetKlinesEntity::GetKlinesEntity(void* service, IKlineCacher* cacher)
: responder_(&ctx_)
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetKlinesEntity::register_call()
{
    std::cout << "register GetKlinesEntity" << std::endl;
    service_->RequestGetKlines(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetKlinesEntity::process()
{
    vector<KlineData> klines;
    cacher_->get_kline(request_.exchange(), request_.symbol(), request_.resolution(), request_.start_time(), request_.end_time(), klines);

    GetKlinesResponse reply;
    reply.set_exchange(request_.exchange());
    reply.set_symbol(request_.symbol());
    reply.set_resolution(request_.resolution());
    reply.set_total_num(klines.size());
    reply.set_num(klines.size());
    for( size_t i = 0 ; i < klines.size() ; i ++ ) 
    {
        kline_to_pbkline(klines[i], reply.add_klines());
    }
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
GetLastEntity::GetLastEntity(void* service, IKlineCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetLastEntity::on_init()
{
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min1;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> cache_min60;
    cacher_->fill_cache(cache_min1, cache_min60);
    tfm::printfln("kline get last registered, init min1/%u min60/%u", cache_min1.size(), cache_min60.size());
    for( const auto& v : cache_min1 ) {
        for( const auto& v2 : v.second ) {
            WrapperKlineData symbol_kline;
            symbol_kline.exchange = v.first;
            symbol_kline.symbol = v2.first;
            symbol_kline.resolution = 60;
            symbol_kline.klines = v2.second;
            datas_.enqueue(symbol_kline);
        }
    }
    for( const auto& v : cache_min60 ) {
        for( const auto& v2 : v.second ) {
            WrapperKlineData symbol_kline;
            symbol_kline.exchange = v.first;
            symbol_kline.symbol = v2.first;
            symbol_kline.resolution = 3600;
            symbol_kline.klines = v2.second;
            datas_.enqueue(symbol_kline);
        }
    }
}

void GetLastEntity::add_data(const WrapperKlineData& data)
{
    datas_.enqueue(data);
}

void GetLastEntity::register_call()
{
    std::cout << "register GetLastEntity" << std::endl;
    service_->RequestGetLast(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastEntity::process()
{    
    MultiGetKlinesResponse reply;
    
    WrapperKlineData ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        const WrapperKlineData& v = ptrs[i];
        GetKlinesResponse* resp = reply.add_data();
        resp->set_exchange(v.exchange);
        resp->set_symbol(v.symbol);
        resp->set_resolution(v.resolution);
        resp->set_num(v.klines.size());
        for( size_t i = 0 ; i < v.klines.size() ; i ++ ) 
        {
            kline_to_pbkline(v.klines[i], resp->add_klines());
        }
    }

    if( reply.data_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

//////////////////////////////////////////////////
SubscribeTradeEntity::SubscribeTradeEntity(void* service)
: responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeTradeEntity::register_call()
{
    std::cout << "register SubscribeTradeEntity" << std::endl;
    service_->RequestSubscribeTrade(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool SubscribeTradeEntity::process()
{    
    MultiTradeWithDecimal reply;
    
    TradePtr ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        TradeWithDecimal* quote = reply.add_trades();
        copy_protobuf_object((TradeWithDecimal*)ptrs[i].get(), quote);
    }

    if( reply.trades_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    } 
}

void SubscribeTradeEntity::add_data(TradePtr data) 
{   
    datas_.enqueue(data);
    //std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    //datas_.push_back(data);
}

//////////////////////////////////////////////////
GetLastTradesEntity::GetLastTradesEntity(void* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetLastTradesEntity::register_call()
{
    std::cout << "register GetLastTradesEntity" << std::endl;
    service_->RequestGetLatestTrades(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastTradesEntity::process()
{
    vector<TradeWithDecimal> trades;
    cacher_->get_latetrades(trades);

    GetLatestTradesResp reply;
    for( size_t i = 0 ; i < trades.size() ; i ++ ) 
    {
        copy_protobuf_object(&trades[i], reply.add_trades());
    }

    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}