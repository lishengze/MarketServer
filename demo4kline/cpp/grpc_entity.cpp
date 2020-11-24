#include "grpc_entity.h"


//////////////////////////////////////////////////
GetKlinesEntity::GetKlinesEntity(void* service, IDataProvider* provider):responder_(&ctx_)
{
    service_ = (GrpcKlineService::AsyncService*)service;
    provider_ = provider;
}

void GetKlinesEntity::register_call(){
    std::cout << "register GetKlinesEntity" << std::endl;
    service_->RequestGetKlines(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetKlinesEntity::process()
{
    vector<KlineData> klines;
    provider_->get_kline(request_.symbol(), request_.resolution(), request_.start_time(), request_.end_time(), klines);

    GetKlinesResponse reply;
    reply.set_symbol("BTC_USDT");
    reply.set_resolution(request_.resolution());
    reply.set_total_num(klines.size());
    reply.set_num(klines.size());
    reply.set_data(&*klines.begin(), klines.size() * sizeof(KlineData));
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
GetLastEntity::GetLastEntity(void* service, IDataProvider* provider):responder_(&ctx_)
{
    service_ = (GrpcKlineService::AsyncService*)service;
    provider_ = provider;
}

void GetLastEntity::register_call(){
    std::cout << "register GetLastEntity" << std::endl;
    service_->RequestGetLast(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastEntity::process()
{    
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    if( datas_.size() == 0 )
        return false;

    KlineData tmp = datas_.front();
    datas_.pop_front();
    inner_lock.unlock();

    
    GetKlinesResponse reply;
    reply.set_symbol("BTC_USDT");
    reply.set_total_num(1);
    reply.set_num(1);
    reply.set_resolution(60);
    reply.set_data(&tmp, sizeof(tmp));
    responder_.Write(reply, this);      
    return true;
}
