#include "grpc_entity.h"
#include "datacenter.h"
#include "converter.h"

void quote_to_quote(const MarketStreamData* src, MarketStreamData* dst) {
    dst->set_exchange(src->exchange());
    dst->set_symbol(src->symbol());
    dst->set_is_snap(src->is_snap());
    dst->set_price_precise(src->price_precise());
    dst->set_volume_precise(src->volume_precise());

    // 卖盘
    for( int i = 0 ; i < src->asks_size() ; ++i ) {
        const Depth& src_depth = src->asks(i);
        Depth* depth = dst->add_asks();
        depth->set_price(src_depth.price());
        depth->set_volume(src_depth.volume());        
        for( auto v : src_depth.data() ) {
            (*depth->mutable_data())[v.first] = v.second;
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bids_size() ; ++i ) {
        const Depth& src_depth = src->bids(i);
        Depth* depth = dst->add_bids();
        depth->set_price(src_depth.price());
        depth->set_volume(src_depth.volume());        
        for( auto v : src_depth.data() ) {
            (*depth->mutable_data())[v.first] = v.second;
        }
    }
};

void quote_to_quote(const MarketStreamDataWithDecimal* src, MarketStreamDataWithDecimal* dst) {
    dst->set_exchange(src->exchange());
    dst->set_symbol(src->symbol());
    dst->set_seq_no(src->seq_no());
    dst->set_price_precise(src->price_precise());
    dst->set_volume_precise(src->volume_precise());
    dst->set_is_snap(src->is_snap());

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

//////////////////////////////////////////////////
MarketStream4BrokerEntity::MarketStream4BrokerEntity(void* service, IDataCacher* cacher)
: responder_(&ctx_)
, cacher_(cacher)
, snap_sended_(false)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4BrokerEntity::register_call(){
    std::cout << "register MarketStream4BrokerEntity" << std::endl;
    service_->RequestServeMarketStream4Broker(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4BrokerEntity::on_init()
{
    
}

bool MarketStream4BrokerEntity::process(){
    
    MultiMarketStreamData reply;

    if( snap_sended_ )
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamData* quote = reply.add_quotes();
            quote_to_quote((MarketStreamData*)datas_[i].get(), quote);
        }
        datas_.clear();
    }
    else
    {
        vector<SInnerQuote> snaps;
        cacher_->get_snaps(snaps);
        for( const auto& v : snaps ) {
            MarketStreamData* quote = reply.add_quotes();
            innerquote_to_msd2(v, quote, true);            
        }
        snap_sended_ = true;
    }
    
    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4BrokerEntity::add_data(std::shared_ptr<void> snap) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}

//////////////////////////////////////////////////
MarketStream4HedgeEntity::MarketStream4HedgeEntity(void* service, IDataCacher* cacher)
: responder_(&ctx_)
, cacher_(cacher)
, snap_sended_(false)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4HedgeEntity::register_call(){
    std::cout << "register MarketStream4HedgeEntity" << std::endl;
    service_->RequestServeMarketStream4Hedge(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4HedgeEntity::on_init()
{
    
}

bool MarketStream4HedgeEntity::process(){
    
    MultiMarketStreamData reply;

    if( snap_sended_ )
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamData* quote = reply.add_quotes();
            quote_to_quote((MarketStreamData*)datas_[i].get(), quote);
        }
        datas_.clear();
    }
    else
    {
        vector<SInnerQuote> snaps;
        cacher_->get_snaps(snaps);
        for( const auto& v : snaps ) {
            MarketStreamData* quote = reply.add_quotes();
            innerquote_to_msd2(v, quote, true);            
        }
        snap_sended_ = true;
    }

    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4HedgeEntity::add_data(std::shared_ptr<void> snap) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}

//////////////////////////////////////////////////
MarketStream4ClientEntity::MarketStream4ClientEntity(void* service, IDataCacher* cacher)
: responder_(&ctx_)
, cacher_(cacher)
, snap_sended_(false)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
}

void MarketStream4ClientEntity::register_call(){
    std::cout << "register MarketStream4ClientEntity" << std::endl;
    service_->RequestServeMarketStream4Client(&ctx_, &request_, &responder_, cq_, cq_, this);
}

void MarketStream4ClientEntity::on_init()
{
    
}

bool MarketStream4ClientEntity::process(){
    
    MultiMarketStreamDataWithDecimal reply;

    if( snap_sended_ )
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

        for( size_t i = 0 ; i < datas_.size() ; ++i ) {
            MarketStreamDataWithDecimal* quote = reply.add_quotes();
            quote_to_quote((MarketStreamDataWithDecimal*)datas_[i].get(), quote);
        }
        datas_.clear();
    }
    else
    {
        vector<SInnerQuote> snaps;
        cacher_->get_snaps(snaps);
        for( const auto& v : snaps ) {
            MarketStreamDataWithDecimal* quote = reply.add_quotes();
            innerquote_to_msd3(v, quote, true);            
        }
        snap_sended_ = true;
    }

    if( reply.quotes_size() > 0 ) {
        responder_.Write(reply, this);      
        return true;
    } else {
        return false;
    }
}

void MarketStream4ClientEntity::add_data(std::shared_ptr<void> snap) {
    std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
    datas_.push_back(snap);
}

//////////////////////////////////////////////////
OtcQuoteEntity::OtcQuoteEntity(void* service, IDataCacher* cacher):responder_(&ctx_)
{
    service_ = (GrpcRiskControllerService::AsyncService*)service;
    cacher_ = cacher;
}

void OtcQuoteEntity::register_call(){
    std::cout << "register OtcQuoteEntity" << std::endl;
    service_->RequestOtcQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
}

bool OtcQuoteEntity::process()
{
    SDecimal price;
    QuoteResponse_Result result = cacher_->otc_query("", request_.symbol(), request_.direction(), request_.amount(), request_.turnover(), price);

    QuoteResponse reply;
    reply.set_symbol(request_.symbol());
    reply.set_price(price.get_str_value());
    reply.set_result(result);
    status_ = FINISH;
    responder_.Finish(reply, Status::OK, this);
    return true;
}
