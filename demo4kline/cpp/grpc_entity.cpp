#include "grpc_entity.h"


//////////////////////////////////////////////////
GetKlinesEntity::GetKlinesEntity(void* service, IDataProvider* provider):responder_(&ctx_)
{
    service_ = (GrpcKlineService::AsyncService*)service;
}

void GetKlinesEntity::register_call(){
    std::cout << "register GetKlinesEntity" << std::endl;
    service_->RequestGetKlines(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetKlinesEntity::process(){
    
    GetKlinesResponse reply;
    
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}

//////////////////////////////////////////////////
GetLastEntity::GetLastEntity(void* service, IDataProvider* provider):responder_(&ctx_)
{
    service_ = (GrpcKlineService::AsyncService*)service;
}

void GetLastEntity::register_call(){
    std::cout << "register GetLastEntity" << std::endl;
    service_->RequestGetLast(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool GetLastEntity::process(){
    
    GetKlinesResponse reply;
    {
        /*std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamData* quote = reply.add_quotes();
            quote_to_quote((MarketStreamData*)datas_[i].get(), quote);
        }
        datas_.clear();*/
    }
    return true;
}

void GetLastEntity::add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}
