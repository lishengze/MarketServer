#include "stream_engine_config.h"
#include "grpc_entity.h"
#include "quote_mixer2.h"

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

GrpcDemoEntity::GrpcDemoEntity(void* service):responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GrpcDemoEntity::register_call(){
    std::cout << "register GrpcDemoEntity" << std::endl;
    service_->RequestDemo(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GrpcDemoEntity::process(){
    times_ ++;
    DemoResp reply;
    reply.set_resp(times_);
    responder_.Write(reply, this);      
    return true;
}

//////////////////////////////////////////////////
SubscribeSingleQuoteEntity::SubscribeSingleQuoteEntity(void* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
, last_seqno(0)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeSingleQuoteEntity::register_call(){
    std::cout << "register SubscribeSingleQuoteEntity" << std::endl;
    service_->RequestSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeSingleQuoteEntity::on_init() 
{
    std::shared_ptr<MarketStreamDataWithDecimal> snap;
    if( cacher_->get_lastsnap(request_.exchange(), request_.symbol(), snap) )
    {            
        datas_.push_back(snap);
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
bool SubscribeSingleQuoteEntity::process(){
    
    MultiMarketStreamDataWithDecimal reply;

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( size_t i = 0 ; i < datas_.size() ; ++i ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)datas_[i].get(), quote);
        last_seqno = quote->seq_no();
        //compare_pb_json2(*quote);
    }
    datas_.clear();

    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    } 
}

void SubscribeSingleQuoteEntity::add_data(SnapAndUpdate data) {
    MarketStreamDataWithDecimal* pdata = (MarketStreamDataWithDecimal*)data.snap.get();    
    if( string(request_.symbol()) != string(pdata->symbol()) || string(request_.exchange()) != string(pdata->exchange()) ) {
        //cout << "filter:" << request_.symbol() << ":" << string(pdata->symbol()) << "," << string(request_.exchange()) << ":" << exchange << endl;
        return;
    }
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    if( data.update ) {
        datas_.push_back(data.update);
    } else {
        //_log_and_print("%s.%s publish snap", pdata->exchange(), pdata->symbol());
        datas_.push_back(data.snap);
    }
}

//////////////////////////////////////////////////
SubscribeMixQuoteEntity::SubscribeMixQuoteEntity(void* service, IMixerCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
, last_seqno(0)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeMixQuoteEntity::register_call(){
    std::cout << "register SubscribeMixQuoteEntity" << std::endl;
    service_->RequestSubscribeMixQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeMixQuoteEntity::on_init() 
{
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    cacher_->get_lastsnaps(snaps);
    tfm::printfln("get_lastsnaps %u items", snaps.size());

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    for( const auto& v : snaps ){
        datas_.push_back(v);
    }
}

bool SubscribeMixQuoteEntity::process()
{    
    MultiMarketStreamDataWithDecimal reply;

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

    for( size_t i = 0 ; i < datas_.size() ; ++i ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)datas_[i].get(), quote);
    }
    datas_.clear();

    // 执行发送
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void SubscribeMixQuoteEntity::add_data(SnapAndUpdate data) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(data.snap);
}

//////////////////////////////////////////////////
SetParamsEntity::SetParamsEntity(void* service):responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SetParamsEntity::register_call(){
    std::cout << "register SetParamsEntity" << std::endl;
    service_->RequestSetParams(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool SetParamsEntity::process()
{    
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
    return true;
}
//////////////////////////////////////////////////
GetParamsEntity::GetParamsEntity(void* service):responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetParamsEntity::register_call(){
    std::cout << "register GetParamsEntity" << std::endl;
    service_->RequestGetParams(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetParamsEntity::process(){    
    GetParamsResp reply;
    reply.set_json_data(CONFIG->get_config());
    
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
GetKlinesEntity::GetKlinesEntity(void* service, IKlineCacher* cacher):responder_(&ctx_)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
    cacher_ = cacher;
}

void GetKlinesEntity::register_call(){
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
GetLastEntity::GetLastEntity(void* service, IKlineCacher* cacher):responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
    cacher_ = cacher;
    snap_cached_ = false;
}

void GetLastEntity::register_call(){
    std::cout << "register GetLastEntity" << std::endl;
    service_->RequestGetLast(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastEntity::_fill_data(MultiGetKlinesResponse& reply)
{
    size_t limit_size = 10000, current_size = 0; // 单次最多发送这个数量

    for( auto iter = cache_min1_.begin() ; iter != cache_min1_.end() ; ) 
    {
        const TExchange& exchange = iter->first;
        for( auto iter2 = iter->second.begin() ; iter2 != iter->second.end() ; ) {
            const TSymbol& symbol = iter2->first;
            const vector<KlineData>& klines = iter2->second;

            // 转换
            GetKlinesResponse* resp = reply.add_data();
            resp->set_exchange(exchange);
            resp->set_symbol(symbol);
            resp->set_resolution(60);
            resp->set_num(klines.size());
            for( size_t i = 0 ; i < klines.size() ; i ++ ) 
            {
                //cout << klines[i].index << endl;
                kline_to_pbkline(klines[i], resp->add_klines());
            }

            // 累计总量
            current_size += klines.size();
            iter->second.erase(iter2++);

            // 终止
            if( current_size >= limit_size )
                return true;
        }

        // 删除
        if( iter->second.size() == 0 ) {
            cache_min1_.erase(iter++);
        } else {
            iter++;
        }

        // 终止
        if( current_size >= limit_size )
            return true;
    }

    for( auto iter = cache_min60_.begin() ; iter != cache_min60_.end() ; ) 
    {
        const TExchange& exchange = iter->first;
        for( auto iter2 = iter->second.begin() ; iter2 != iter->second.end() ; ) {
            const TSymbol& symbol = iter2->first;
            const vector<KlineData>& klines = iter2->second;

            // 转换
            GetKlinesResponse* resp = reply.add_data();
            resp->set_exchange(exchange);
            resp->set_symbol(symbol);
            resp->set_resolution(3600);
            resp->set_num(klines.size());
            for( size_t i = 0 ; i < klines.size() ; i ++ ) 
            {
                kline_to_pbkline(klines[i], resp->add_klines());
            }

            // 累计总量
            current_size += klines.size();
            iter->second.erase(iter2++);

            // 终止
            if( current_size >= limit_size )
                return true;
        }

        // 删除
        if( iter->second.size() == 0 ) {
            cache_min60_.erase(iter++);
        } else {
            iter++;
        }

        // 终止
        if( current_size >= limit_size )
            return true;
    }

    return current_size > 0;
}

bool GetLastEntity::process()
{    
    if( !_snap_sended() )
    {
        if( !snap_cached_ ){
            cacher_->fill_cache(cache_min1_, cache_min60_);
            tfm::printfln("kline get last registered, init min1/%u min60/%u", cache_min1_.size(), cache_min60_.size());
            snap_cached_ = true;
        } 

        MultiGetKlinesResponse reply;
        if( _fill_data(reply) ) {
            responder_.Write(reply, this);      
            return true;
        } else {
            return false;
        }
    }

    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    if( datas_.size() == 0 )
        return false;
        
    MultiGetKlinesResponse reply;
    for( const auto& v : datas_ )
    {
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
    datas_.clear();

    inner_lock.unlock();
    responder_.Write(reply, this);      
    return true;
}

//////////////////////////////////////////////////
SubscribeTradeEntity::SubscribeTradeEntity(void* service):responder_(get_context())
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
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {            
            TradeWithDecimal* quote = reply.add_trades();
            copy_protobuf_object((TradeWithDecimal*)datas_[i].get(), quote);
        }
        datas_.clear();
    }
    if( reply.trades_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    } 
}

void SubscribeTradeEntity::add_data(std::shared_ptr<TradeWithDecimal> data) 
{   
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(data);
}

//////////////////////////////////////////////////
GetLastTradesEntity::GetLastTradesEntity(void* service, IQuoteCacher* cacher):responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
    cacher_ = cacher;
}

void GetLastTradesEntity::register_call(){
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