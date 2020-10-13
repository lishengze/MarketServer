#include "datacenter.h"
#include "grpc_caller.h"

void innerquote_to_msd(const SInnerQuote& quote, MarketStreamData* msd) 
{
    msd->set_symbol(quote.symbol);
    msd->set_time(quote.time);
    msd->set_time_arrive(quote.time_arrive);
    char sequence[256];
    sprintf(sequence, "%lld", quote.seq_no);
    msd->set_msg_seq(sequence);
    // 卖盘
    for( int i = 0 ; i < quote.ask_length ; ++i ) {
        const SInnerDepth& srcDepth = quote.asks[i];
        Depth* depth = msd->add_ask_depth();        
        depth->set_price(srcDepth.price.GetStrValue());
        for( int j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_exchange(srcDepth.exchanges[j].name);
            depthData->set_size(srcDepth.exchanges[j].volume);
        }
    }
    // 买盘
    for( int i = 0 ; i < quote.bid_length ; ++i ) {
        const SInnerDepth& srcDepth = quote.bids[i];
        Depth* depth = msd->add_bid_depth();        
        depth->set_price(srcDepth.price.GetStrValue());
        for( int j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_exchange(srcDepth.exchanges[j].name);
            depthData->set_size(srcDepth.exchanges[j].volume);
        }
    }
}

DataCenter::DataCenter() {

}

DataCenter::~DataCenter() {

}

void DataCenter::add_quote(const SInnerQuote& quote)
{
    // SInnerQuote -> MarketStreamData
    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd(quote, ptrData.get());

    // send to clients
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
        iter->first->add_data(ptrData);
    }
};

void DataCenter::change_account(const AccountInfo& info)
{

}

void DataCenter::change_configuration(const QuoteConfiguration& config)
{

}

void DataCenter::add_client(CallDataServeMarketStream* client)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    clients_[client] = true;
}

void DataCenter::del_client(CallDataServeMarketStream* client)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    auto iter = clients_.find(client);
    if( iter != clients_.end() ) {
        clients_.erase(iter);
    }
}
