#include "stream_engine_config.h"
#include "grpc_entity.h"

void quote_to_quote(const QuoteData* src, QuoteData* dst) {
    dst->set_symbol(src->symbol());
    dst->set_exchange(src->exchange());
    dst->set_msg_seq(src->msg_seq());

    // 卖盘
    for( int i = 0 ; i < src->ask_depth_size() ; ++i ) {
        const DepthLevel& src_depth = src->ask_depth(i);
        DepthLevel* dst_depth = dst->add_ask_depth();
        dst_depth->mutable_price()->set_value(src_depth.price().value());
        dst_depth->mutable_price()->set_base(src_depth.price().base());
        dst_depth->set_volume(src_depth.volume());
    }
    // 买盘
    for( int i = 0 ; i < src->bid_depth_size() ; ++i ) {
        const DepthLevel& src_depth = src->bid_depth(i);
        DepthLevel* dst_depth = dst->add_bid_depth();
        dst_depth->mutable_price()->set_value(src_depth.price().value());
        dst_depth->mutable_price()->set_base(src_depth.price().base());
        dst_depth->set_volume(src_depth.volume());
    }
};

void quote_to_quote(const MarketStreamData* src, MarketStreamData* dst) {
    dst->set_symbol(src->symbol());
    dst->set_msg_seq(src->msg_seq());

    // 卖盘
    for( int i = 0 ; i < src->ask_depths_size() ; ++i ) {
        const Depth& src_depth = src->ask_depths(i);
        Depth* dst_depth = dst->add_ask_depths();
        dst_depth->set_price(src_depth.price());
        dst_depth->set_volume(src_depth.volume());     
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bid_depths_size() ; ++i ) {
        const Depth& src_depth = src->bid_depths(i);
        Depth* dst_depth = dst->add_bid_depths();
        dst_depth->set_price(src_depth.price());
        dst_depth->set_volume(src_depth.volume());     
        for( auto v : src_depth.data() ) {
            (*dst_depth->mutable_data())[v.first] = v.second;
        }
    }
};

GrpcDemoEntity::GrpcDemoEntity(void* service):responder_(&ctx_)
{
    service_ = (StreamEngineService::AsyncService*)service;
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
    service_ = (StreamEngineService::AsyncService*)service;
}

void SubscribeSingleQuoteEntity::register_call(){
    std::cout << "register SubscribeSingleQuoteEntity" << std::endl;
    service_->RequestSubscribeOneQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool SubscribeSingleQuoteEntity::process(){
    
    MultiQuoteData reply;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            QuoteData* quote = reply.add_quotes();
            quote_to_quote((QuoteData*)datas_[i].get(), quote);
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
    QuoteData* pdata = (QuoteData*)snap.get();    
    if( string(request_.symbol()) != string(pdata->symbol()) || string(request_.exchange()) != string(pdata->exchange()) ) {
        //cout << "filter:" << request_.symbol() << ":" << string(pdata->symbol()) << "," << string(request_.exchange()) << ":" << exchange << endl;
        return;
    }
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}

//////////////////////////////////////////////////
SubscribeMixQuoteEntity::SubscribeMixQuoteEntity(void* service):responder_(&ctx_)
{
    service_ = (StreamEngineService::AsyncService*)service;
}

void SubscribeMixQuoteEntity::register_call(){
    std::cout << "register SubscribeMixQuoteEntity" << std::endl;
    service_->RequestMultiSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
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
    service_ = (StreamEngineService::AsyncService*)service;
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
        CONFIG->symbol_precise_[current_symbol] = request_.precise();
    }
    
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
    return true;
}
//////////////////////////////////////////////////
GetParamsEntity::GetParamsEntity(void* service):responder_(&ctx_)
{
    service_ = (StreamEngineService::AsyncService*)service;
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