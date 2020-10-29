#include "grpc_entity.h"

void quote_to_quote(const MarketStreamData* src, MarketStreamData* dst) {
    dst->set_symbol(src->symbol());
    dst->set_msg_seq(src->msg_seq());

    // 卖盘
    for( int i = 0 ; i < src->ask_depths_size() ; ++i ) {
        const Depth& src_depth = src->ask_depths(i);
        Depth* depth = dst->add_ask_depths();
        depth->set_price(src_depth.price());
        depth->set_volume(src_depth.volume());        
        for( auto v : src_depth.data() ) {
            (*depth->mutable_data())[v.first] = v.second;
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bid_depths_size() ; ++i ) {
        const Depth& src_depth = src->bid_depths(i);
        Depth* depth = dst->add_bid_depths();
        depth->set_price(src_depth.price());
        depth->set_volume(src_depth.volume());        
        for( auto v : src_depth.data() ) {
            (*depth->mutable_data())[v.first] = v.second;
        }
    }
};

//////////////////////////////////////////////////
MarketStreamEntity::MarketStreamEntity(void* service):responder_(&ctx_)
{
    service_ = (Broker::AsyncService*)service;
}

void MarketStreamEntity::register_call(){
    std::cout << "register MarketStreamEntity" << std::endl;
    service_->RequestServeMarketStream(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool MarketStreamEntity::process(){
    
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

void MarketStreamEntity::add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}
