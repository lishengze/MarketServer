#include "grpc_caller.h"
#include "grpc_server.h"

void quote_to_quote(const MarketStreamData* src, MarketStreamData* dst) {
    dst->set_symbol(src->symbol());
    dst->set_msg_seq(src->msg_seq());

    // 卖盘
    for( int i = 0 ; i < src->ask_depth_size() ; ++i ) {
        const Depth& srcDepth = src->ask_depth(i);
        Depth* depth = dst->add_ask_depth();
        depth->set_price(srcDepth.price());
        for( int j = 0 ; j < srcDepth.data_size() ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_size(srcDepth.data(j).size());
            depthData->set_exchange(srcDepth.data(j).exchange());
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bid_depth_size() ; ++i ) {
        const Depth& srcDepth = src->bid_depth(i);
        Depth* depth = dst->add_bid_depth();
        depth->set_price(srcDepth.price());
        for( int j = 0 ; j < srcDepth.data_size() ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_size(srcDepth.data(j).size());
            depthData->set_exchange(srcDepth.data(j).exchange());
        }
    }
};

void CallDataServeMarketStream::Release() {
    std::cout << "delete CallDataServeMarketStream" << std::endl;
    parent_->unregister_client(this);
    delete this;
}

void CallDataServeMarketStream::Proceed() {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestServeMarketStream(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
        //std::cout << "CallDataServeMarketStream " << times_ << " " << status_ << std::endl;
        if (times_ == 0)
        {
            parent_->register_client(this);
            new CallDataServeMarketStream(service_, cq_, parent_);
        }
        
        if (times_++ < 0 )
        {
            status_ = FINISH;
            responder_.Finish(Status::OK, this);
        }
        else
        {
            MultiMarketStreamData reply;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

                for( size_t i = 0 ; i < datas_.size() ; ++i ) {
                    MarketStreamData* quote = reply.add_quotes();
                    quote_to_quote(datas_[i].get(), quote);
                }
                datas_.clear();
            }
            
            if( reply.quotes_size() > 0 ) {
                std::cout << "get " << reply.quotes_size() << " items" << std::endl;
                responder_.Write(reply, this);               
            } else {
                alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
            }

            status_ = PUSH_TO_BACK;
        }
    } else if(status_ == PUSH_TO_BACK) {
        status_ = PROCESS;
        alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
    } else {
        std::cout << "delete CallDataServeMarketStream" << std::endl;
        parent_->unregister_client(this);
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
};
