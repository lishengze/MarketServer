#include "stream_engine_config.h"
#include "grpc_entity.h"

void quote_to_quote(const MarketStreamData* src, MarketStreamData* dst) {
    dst->set_exchange(src->exchange());
    dst->set_symbol(src->symbol());
    dst->set_seq_no(src->seq_no());
    dst->set_is_snap(src->is_snap());

    // 卖盘
    for( int i = 0 ; i < src->asks_size() ; ++i ) {
        const Depth& src_depth = src->asks(i);
        Depth* dst_depth = dst->add_asks();
        dst_depth->set_price(src_depth.price());
        dst_depth->set_volume(src_depth.volume());     
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bids_size() ; ++i ) {
        const Depth& src_depth = src->bids(i);
        Depth* dst_depth = dst->add_bids();
        dst_depth->set_price(src_depth.price());
        dst_depth->set_volume(src_depth.volume());     
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
};

GrpcDemoEntity::GrpcDemoEntity(void* service):responder_(&ctx_)
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
SubscribeSingleQuoteEntity::SubscribeSingleQuoteEntity(void* service):responder_(&ctx_)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
    snap_sended_ = false;
    last_seqno = 0;
}

void SubscribeSingleQuoteEntity::register_call(){
    std::cout << "register SubscribeSingleQuoteEntity" << std::endl;
    service_->RequestSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void compare_pb_json2(const MarketStreamData& quote)
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
        MarketStreamData tmp;
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

bool SubscribeSingleQuoteEntity::process(){
    
    MultiMarketStreamData reply;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamData* quote = reply.add_quotes();
            quote_to_quote((MarketStreamData*)datas_[i].get(), quote);
            // 检查seq_no
            //if( last_seqno != 0 && (1+last_seqno) != quote->seq_no() ) {
            //    cout << "lost !!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
            //}
            last_seqno = quote->seq_no();
            //compare_pb_json2(*quote);
        }
        datas_.clear();
    }
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    } 
}

void SubscribeSingleQuoteEntity::add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) {
    MarketStreamData* pdata = (MarketStreamData*)snap.get();    
    if( string(request_.symbol()) != string(pdata->symbol()) || string(request_.exchange()) != string(pdata->exchange()) ) {
        //cout << "filter:" << request_.symbol() << ":" << string(pdata->symbol()) << "," << string(request_.exchange()) << ":" << exchange << endl;
        return;
    }
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    if( snap_sended_ && update ) {
        datas_.push_back(update);
    } else {
        snap_sended_ = true;
        datas_.push_back(snap);
    }
}

//////////////////////////////////////////////////
SubscribeMixQuoteEntity::SubscribeMixQuoteEntity(void* service):responder_(&ctx_)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
    snap_sended_ = false;
}

void SubscribeMixQuoteEntity::register_call(){
    std::cout << "register SubscribeMixQuoteEntity" << std::endl;
    service_->RequestSubscribeMixQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool SubscribeMixQuoteEntity::process(){
    
    MultiMarketStreamData reply;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamData* quote = reply.add_quotes();
            quote_to_quote((MarketStreamData*)datas_[i].get(), quote);
        }
        datas_.clear();
    }
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void SubscribeMixQuoteEntity::add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}

//////////////////////////////////////////////////
SetParamsEntity::SetParamsEntity(void* service):responder_(&ctx_)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void SetParamsEntity::register_call(){
    std::cout << "register SetParamsEntity" << std::endl;
    service_->RequestSetParams(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool SetParamsEntity::process(){
    
    CONFIG->grpc_publish_frequency_ = request_.frequency();
    CONFIG->grpc_publish_depth_ = request_.depth();
    CONFIG->grpc_publish_raw_frequency_ = request_.raw_frequency();
    string current_symbol = request_.symbol();
    if( current_symbol != "" ) {
        unordered_map<string, int> vals;
        vals[current_symbol] = request_.precise();
        CONFIG->set_configuration_precise(vals);
    }
    
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
    return true;
}
//////////////////////////////////////////////////
GetParamsEntity::GetParamsEntity(void* service):responder_(&ctx_)
{
    service_ = (GrpcStreamEngineService::AsyncService*)service;
}

void GetParamsEntity::register_call(){
    std::cout << "register GetParamsEntity" << std::endl;
    service_->RequestGetParams(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetParamsEntity::process(){    
    GetParamsResp reply;
    reply.set_depth(CONFIG->grpc_publish_depth_);
    reply.set_frequency(CONFIG->grpc_publish_frequency_);
    reply.set_raw_frequency(CONFIG->grpc_publish_raw_frequency_);
    for( auto v : CONFIG->include_symbols_ ) {
        reply.add_symbols(v);
    }
    for( auto v : CONFIG->include_exchanges_ ) {
        reply.add_exchanges(v);
    }
    
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}