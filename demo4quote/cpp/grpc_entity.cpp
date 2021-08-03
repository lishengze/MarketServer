#include "stream_engine_config.h"
#include "grpc_entity.h"
#include "quote_mixer2.h"
#include "Log/log.h"

#define ONE_ROUND_MESSAGE_NUMBRE 999

template<class T>
void copy_protobuf_object(const T* src, T* dst) {
    string tmp;
    if( !src->SerializeToString(&tmp) )
        return;
    dst->ParseFromString(tmp);
}

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
/*
SubscribeSingleQuoteEntity::SubscribeSingleQuoteEntity(void* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeSingleQuoteEntity::register_call()
{
    _log_and_print("%s register SubscribeSingleQuoteEntity", get_context()->peer());
    service_->RequestSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeSingleQuoteEntity::on_init() 
{
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    if( cacher_->get_lastsnaps(snaps) )
    {            
        for( const auto& v : snaps ) {
            if( _is_filtered(v->exchange(), v->symbol()) )
                continue;
            datas_.enqueue(v);
        }
    }
}

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

bool SubscribeSingleQuoteEntity::process()
{
    MultiMarketStreamDataWithDecimal reply;
    type_tick now = get_miliseconds();

    StreamDataPtr ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)ptrs[i].get(), quote);
        TimeCostWatcher w(tfm::format("grpc-%u", quote->time_produced_by_streamengine()), quote->time_produced_by_streamengine());
        quote->set_time_produced_by_streamengine(now);
    }    
    type_tick end = get_miliseconds();
    if( (end - now) > 5 ) {
        tfm::printfln("seriealize %u object cost %u", count, end-now);
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
SubscribeMixQuoteEntity::SubscribeMixQuoteEntity(void* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeMixQuoteEntity::register_call()
{
    _log_and_print("%s register SubscribeMixQuoteEntity", get_context()->peer());
    service_->RequestSubscribeMixQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeMixQuoteEntity::on_init() 
{
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    TExchange exchange = "";
    cacher_->get_lastsnaps(snaps, &exchange);
    tfm::printfln("get_lastsnaps %u items", snaps.size());

    for( const auto& v : snaps ){
        datas_.enqueue(v);
    }
}

bool SubscribeMixQuoteEntity::process()
{    
    MultiMarketStreamDataWithDecimal reply;
    reply.mutable_quotes()->Reserve(ONE_ROUND_MESSAGE_NUMBRE);
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
}*/

//////////////////////////////////////////////////
GetParamsEntity::GetParamsEntity(::grpc::Service* service)
: responder_(get_context())
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetParamsEntity::register_call()
{
    _log_and_print("%s register GetParamsEntity", get_context()->peer());
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
GetKlinesEntity::GetKlinesEntity(::grpc::Service* service, IKlineCacher* cacher)
: responder_(&ctx_)
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetKlinesEntity::register_call()
{
    _log_and_print("%s register GetKlinesEntity", get_context()->peer());
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
GetLastEntity::GetLastEntity(::grpc::Service* service, IKlineCacher* cacher)
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
    tfm::printfln("\n\n------------ kline get last registered, init min1/%u min60/%u", cache_min1.size(), cache_min60.size());

    // cout << "min1_cache_: " << endl;
    // for (auto iter1:cache_min1)
    // {
    //     for (auto iter2:iter1.second)
    //     {
    //         cout << iter1.first << " " << iter2.first << " " << iter2.second.size() << endl;
    //     }
    // }

    // cout << "min60_cache_: " << endl;
    // for (auto iter1:cache_min60)
    // {
    //     for (auto iter2:iter1.second)
    //     {
    //         vector<KlineData>& detail_kline_data = iter2.second;

    //         cout << iter1.first << " " << iter2.first << " " << detail_kline_data.size() << endl;
    //     }
    // }

    int data1_count = 0;
    for( const auto& v : cache_min1 ) {
        for( const auto& v2 : v.second ) {
            WrapperKlineData symbol_kline;
            symbol_kline.exchange = v.first;
            symbol_kline.symbol = v2.first;
            symbol_kline.resolution = 60;
            symbol_kline.klines = v2.second;
            datas_.enqueue(symbol_kline);
            data1_count++;
        }
    }

    int data60_count = 0;
    for( const auto& v : cache_min60 ) {
        for( const auto& v2 : v.second ) {
            WrapperKlineData symbol_kline;
            symbol_kline.exchange = v.first;
            symbol_kline.symbol = v2.first;
            symbol_kline.resolution = 3600;
            symbol_kline.klines = v2.second;
            datas_.enqueue(symbol_kline);
            data60_count++;
        }
    }

    cout << "GetLastEntity::on_init data1_count: " << data1_count << ", data60_count: " 
         << data60_count << ", datas_.size:  " << datas_.size_approx() << "\n\n" << endl;
}

void GetLastEntity::add_data(const WrapperKlineData& data)
{
    datas_.enqueue(data);

    cout << "GetLastEntity::add_data " << datas_.size_approx() << endl;
}

void GetLastEntity::register_call()
{
    _log_and_print("%s register GetLastEntity", get_context()->peer());
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

    // cout << "**** [kline] reply.size: " << reply.data_size() << endl;

    if( reply.data_size() > 0 ) {
        LOG->record_output_info("GRPC_kline");

        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

//////////////////////////////////////////////////
GetLastTradesEntity::GetLastTradesEntity(::grpc::Service* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetLastTradesEntity::register_call()
{
    _log_and_print("%s register GetLastTradesEntity", get_context()->peer());
    service_->RequestGetLatestTrades(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastTradesEntity::process()
{
    vector<std::shared_ptr<TradeWithDecimal>> trades;
    cacher_->get_latetrades(trades);

    GetLatestTradesResp reply;
    for( size_t i = 0 ; i < trades.size() ; i ++ ) 
    {
        copy_protobuf_object(trades[i].get(), reply.add_trades());
    }

    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
SubscribeQuoteInBinaryEntity::SubscribeQuoteInBinaryEntity(::grpc::Service* service, IQuoteCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SubscribeQuoteInBinaryEntity::register_call()
{
    // _log_and_print("%s register SubscribeQuoteInBinaryEntity", get_context()->peer());
    service_->RequestSubscribeQuoteInBinary(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void SubscribeQuoteInBinaryEntity::on_init() 
{
    
    vector<std::shared_ptr<MarketStreamDataWithDecimal>> snaps;
    cacher_->get_lastsnaps(snaps);
    // tfm::printfln("get_lastsnaps %u items", snaps.size());
    // cout << "\nSubscribeQuoteInBinaryEntity::on_init get_lastsnaps " << snaps.size() << endl;
    
    for( const auto& v : snaps ){
        string tmp;
        v->SerializeToString(&tmp);

        uint32 length = tmp.size();
        uint32 data_type = 1;
        string data = tfm::format("%u;%u;", length, data_type);
        data.insert(data.end(), tmp.begin(), tmp.end());

        // add_data(v->exchange(), v->symbol(), data);
    }
}

bool SubscribeQuoteInBinaryEntity::process()
{    
    if (is_inner_write_)
    {
        is_inner_write_ = false;
        return false;
    }    

    // cout << "[depth] Sub Process: ";

    string& data_ref = (*reply_.mutable_data());
    data_ref.clear();

    string ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);

    // if (is_inner_write_)
    // {
    //     is_inner_write_ = false;
    //     cout << "Sub is_inner_write_ " 
    //          << ", data.size: " << count
    //          << ", left_data_size: " << datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE) << endl;
    //     return false;
    // }
    // else
    // {
    //     cout << "data.size: " << count
    //          << ", left_data_size: " << datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE) << endl;
    // }

    // cout << "data.size: " << count
    //         << ", left_data_size: " << datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE) << endl;

    for( size_t i = 0 ; i < count ; i ++ ) {
        data_ref.insert(data_ref.end(), ptrs[i].begin(), ptrs[i].end());
    }  

    // cout << "reply.data.len: " << reply_.data().length()  << "\n" << endl;

    // 执行发送
    if( reply_.data().length() > 0 ) {
        
        is_inner_write_ = true;
        LOG->record_output_info("GRPC_depth_trade");
        responder_.Write(reply_, this);      
        return true;
    } else {
        return false;
    }
}

void SubscribeQuoteInBinaryEntity::add_data(const TExchange& exchange, const TSymbol& symbol, const string& data) 
{
    if( request_.exchange() != "" && exchange != request_.exchange() )
        return;

    // if( request_.symbol() != "" && symbol != request_.symbol() )
    //     return;

    // std::cout << "Sub::add_data " << "request.ex: " << request_.exchange() << ", ex: " << exchange << " "
    //                               << "reqeust.sy: " << request_.symbol() << ", sy: " << symbol
    //                               << endl;

    // std::cout << "SubQuote: " << exchange << ", " << symbol << ", " << endl;

    string ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    
    datas_.enqueue(data);

    // cout << "Sub::add_data.size:  " << datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE) << "\n" << endl;

    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // process();
}