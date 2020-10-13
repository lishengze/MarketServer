#include "grpc_caller.h"
#include "grpc_server.h"

void quote_to_quote(const QuoteData* src, QuoteData* dst) {
    dst->set_symbol(src->symbol());
    dst->set_msg_seq(src->msg_seq());

    // 卖盘
    for( int i = 0 ; i < src->ask_depth_size() ; ++i ) {
        const DepthLevel& srcDepth = src->ask_depth(i);
        DepthLevel* depth = dst->add_ask_depth();
        depth->mutable_price()->set_value(srcDepth.price().value());
        depth->mutable_price()->set_base(srcDepth.price().base());
        for( int j = 0 ; j < srcDepth.data_size() ; ++j ) {
            DepthVolume* depthVolume = depth->add_data();
            depthVolume->set_volume(srcDepth.data(j).volume());
            depthVolume->set_exchange(srcDepth.data(j).exchange());
            
        }
    }
    // 买盘
    for( int i = 0 ; i < src->bid_depth_size() ; ++i ) {
        const DepthLevel& srcDepth = src->bid_depth(i);
        DepthLevel* depth = dst->add_bid_depth();
        depth->mutable_price()->set_value(srcDepth.price().value());
        depth->mutable_price()->set_base(srcDepth.price().base());
        for( int j = 0 ; j < srcDepth.data_size() ; ++j ) {
            DepthVolume* depthVolume = depth->add_data();
            depthVolume->set_volume(srcDepth.data(j).volume());
            depthVolume->set_exchange(srcDepth.data(j).exchange());
            
        }
    }
};

void depthquote_to_quote(const string& exchange, const string& symbol, const SDepthQuote& quote, QuoteData* msd) {
    msd->set_symbol(symbol);
    msd->set_msg_seq(quote.SequenceNo);

    // 卖盘
    {
        for( int i = 0 ; i < quote.AskLength ; ++i ) {
            const SDepthPrice& price = quote.Asks[i];
            DepthLevel* depth = msd->add_ask_depth();
            depth->mutable_price()->set_value(price.Price.Value);
            depth->mutable_price()->set_base(price.Price.Base);
            DepthVolume* depthVolume = depth->add_data();
            depthVolume->set_volume(price.Volume);
            depthVolume->set_exchange(exchange);
        }
    }
    // 买盘
    {
        for( int i = 0 ; i < quote.BidLength ; ++i ) {
            const SDepthPrice& price = quote.Bids[i];
            DepthLevel* depth = msd->add_bid_depth();
            depth->mutable_price()->set_value(price.Price.Value);
            depth->mutable_price()->set_base(price.Price.Base);
            DepthVolume* depthVolume = depth->add_data();
            depthVolume->set_volume(price.Volume);
            depthVolume->set_exchange(exchange);
        }
    }
};

void CallDataGetQuote::Release() {
    std::cout << "delete CallDataGetQuote" << std::endl;
    delete this;
}

void CallDataGetQuote::Proceed() 
{
    if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different CallData
        // instances can serve different requests concurrently), in this case
        // the memory address of this CallData instance.
        service_->RequestGetQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {        
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallDataGetQuote(service_, cq_, parent_);

        // The actual processing.
        QuoteData reply;
        SDepthQuote quote;
        if( parent_->get_snap(request_.exchange(), request_.symbol(), quote) ){
            depthquote_to_quote(request_.exchange(), request_.symbol(), quote, &reply);
        }

        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply, Status::OK, this);
    } else {            
        std::cout << "delete CallDataGetQuote" << std::endl;
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
    }
};

void CallDataMultiSubscribeQuote::Release() {
    std::cout << "delete CallDataMultiSubscribeQuote" << std::endl;
    parent_->unregister_client(this);
    delete this;
}

void CallDataMultiSubscribeQuote::Proceed() {
    if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestMultiSubscribeQuote(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
        //std::cout << "CallDataMultiSubscribeQuote " << times_ << " " << status_ << std::endl;
        if (times_ == 0)
        {
            parent_->register_client(this);
            new CallDataMultiSubscribeQuote(service_, cq_, parent_);
        }
        
        if (times_++ < 0 )
        {
            status_ = FINISH;
            responder_.Finish(Status::OK, this);
        }
        else
        {
            MultiQuoteData reply;
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };

                for( int i = 0 ; i < datas_.size() ; ++i ) {
                    QuoteData* quote = reply.add_quotes();
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
        std::cout << "delete CallDataMultiSubscribeQuote" << std::endl;
        parent_->unregister_client(this);
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
};
