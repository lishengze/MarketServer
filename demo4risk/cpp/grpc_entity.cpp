#include "grpc_entity.h"
#include "datacenter.h"
#include "converter.h"

#define ONE_ROUND_MESSAGE_NUMBRE 999

template<class T>
void copy_protobuf_object(const T* src, T* dst) {
    string tmp;
    if( !src->SerializeToString(&tmp) )
        return;
    dst->ParseFromString(tmp);
}

//////////////////////////////////////////////////
MarketStream4BrokerEntity::MarketStream4BrokerEntity(::grpc::Service* service, IDataCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4BrokerEntity::register_call()
{
    _log_and_print("%s register MarketStream4BrokerEntity", get_context()->peer());
    service_->RequestServeMarketStream4Broker(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4BrokerEntity::on_init()
{    
    vector<SInnerQuote> snaps;
    cacher_->get_snaps(snaps);
    for( const auto& v : snaps ) {
        StreamDataPtr2 ptrData(new MarketStreamData);
        innerquote_to_msd2(v, ptrData.get(), true);
        datas_.enqueue(ptrData);
    }    
}

bool MarketStream4BrokerEntity::process()
{    
    MultiMarketStreamData reply;

    StreamDataPtr2 ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamData* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamData*)ptrs[i].get(), quote);
    }
    
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4BrokerEntity::add_data(StreamDataPtr2 snap) 
{
    datas_.enqueue(snap);
}

//////////////////////////////////////////////////
MarketStream4HedgeEntity::MarketStream4HedgeEntity(::grpc::Service* service, IDataCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4HedgeEntity::register_call()
{
    _log_and_print("%s register MarketStream4HedgeEntity", get_context()->peer());
    service_->RequestServeMarketStream4Hedge(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4HedgeEntity::on_init()
{    
    vector<SInnerQuote> snaps;
    cacher_->get_snaps(snaps);
    for( const auto& v : snaps ) {
        StreamDataPtr2 ptrData(new MarketStreamData);
        innerquote_to_msd2(v, ptrData.get(), true);
        datas_.enqueue(ptrData);
    }    
}

bool MarketStream4HedgeEntity::process()
{    
    MultiMarketStreamData reply;

    StreamDataPtr2 ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamData* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamData*)ptrs[i].get(), quote);
    }    

    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4HedgeEntity::add_data(StreamDataPtr2 snap) 
{
    datas_.enqueue(snap);
}

//////////////////////////////////////////////////
MarketStream4ClientEntity::MarketStream4ClientEntity(::grpc::Service* service, IDataCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4ClientEntity::register_call()
{
    _log_and_print("%s register MarketStream4ClientEntity", get_context()->peer());
    service_->RequestServeMarketStream4Client(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4ClientEntity::on_init()
{
    vector<SInnerQuote> snaps;
    cacher_->get_snaps(snaps);
    for( const auto& v : snaps ) {
        StreamDataPtr ptrData(new MarketStreamDataWithDecimal);
        innerquote_to_msd3(v, ptrData.get(), true);
        datas_.enqueue(ptrData);
    }    
}

bool MarketStream4ClientEntity::process()
{
    MultiMarketStreamDataWithDecimal reply;
    type_tick now = get_miliseconds();

    StreamDataPtr ptrs[ONE_ROUND_MESSAGE_NUMBRE];
    size_t count = datas_.try_dequeue_bulk(ptrs, ONE_ROUND_MESSAGE_NUMBRE);
    for( size_t i = 0 ; i < count ; i ++ ) {
        MarketStreamDataWithDecimal* quote = reply.add_quotes();
        copy_protobuf_object((MarketStreamDataWithDecimal*)ptrs[i].get(), quote);
        quote->set_time_produced_by_riskcontrol(now);
    }
    
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4ClientEntity::add_data(StreamDataPtr snap) 
{
    datas_.enqueue(snap);
}

//////////////////////////////////////////////////
OtcQuoteEntity::OtcQuoteEntity(::grpc::Service* service, IDataCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void OtcQuoteEntity::register_call()
{
    _log_and_print("%s register OtcQuoteEntity", get_context()->peer());
    service_->RequestOtcQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool OtcQuoteEntity::process()
{
    SDecimal price;
    std::cout << "OTC: " << request_.symbol() << " direction: " << request_.direction() << " amount: " << request_.amount() << " "
              << "turnover: " << request_.turnover() << std::endl;

    QuoteResponse_Result result = cacher_->otc_query("", request_.symbol(), request_.direction(), request_.amount(), request_.turnover(), price);

    QuoteResponse reply;
    reply.set_symbol(request_.symbol());
    reply.set_price(price.get_str_value());
    reply.set_result(result);
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
GetParamsEntity::GetParamsEntity(::grpc::Service* service, IDataCacher* cacher)
: responder_(get_context())
, cacher_(cacher)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void GetParamsEntity::register_call()
{
    _log_and_print("%s register GetParamsEntity", get_context()->peer());
    service_->RequestGetParams(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetParamsEntity::process()
{
    map<TSymbol, SDecimal> watermarks;
    map<TExchange, map<TSymbol, double>> accounts;
    map<TSymbol, string> configurations;
    cacher_->get_params(watermarks, accounts, configurations);

    GetParamsResponse reply;
    for( const auto& v : watermarks ) {
        Decimal tmp;        
        set_decimal(&tmp, v.second);
        (*reply.mutable_watermarks())[v.first] = tmp;
    }
    for( const auto& v : accounts ) {
        for( const auto& v2 : v.second ) {
            (*reply.mutable_accounts())[v.first + "." + v2.first] = v2.second;
        }
    }
    for( const auto& v : configurations ) {
        (*reply.mutable_configuration())[v.first] = v.second;
    }
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}
