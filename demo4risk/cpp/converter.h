#pragma once

#include "grpc_entity.h"
#include "datacenter.h"

inline void innerquote_to_msd2(const SInnerQuote& quote, MarketStreamData* msd, bool check_total_volume) 
{
    msd->set_symbol(quote.symbol);
    msd->set_is_snap(true);
    msd->set_price_precise(quote.precise);
    msd->set_volume_precise(quote.vprecise);
    //msd->set_time(quote.time);
    //msd->set_time_arrive(quote.time_arrive);
    //char sequence[256];
    //sprintf(sequence, "%lld", quote.seq_no);
    //msd->set_msg_seq(sequence);
    // 卖盘
    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        Depth* depth = msd->add_asks();        
        depth->set_price(iter->first.get_str_value());
        for( const auto& v : iter->second.exchanges ) {
            if( v.second.is_zero() )
                continue;
             (*depth->mutable_data())[v.first] = v.second.get_value();
        }
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
            continue;
        Depth* depth = msd->add_bids();        
        depth->set_price(iter->first.get_str_value());
        for( const auto& v : iter->second.exchanges ) {
            if( v.second.is_zero() )
                continue;
             (*depth->mutable_data())[v.first] = v.second.get_value();
        }
    }
}

inline void innerquote_to_msd3(const SInnerQuote& quote, MarketStreamDataWithDecimal* msd, bool check_total_volume) 
{
    msd->set_symbol(quote.symbol);
    msd->set_is_snap(true);
    msd->set_price_precise(quote.precise);
    msd->set_volume_precise(quote.vprecise);
    //msd->set_time(quote.time);
    //msd->set_time_arrive(quote.time_arrive);
    //char sequence[256];
    //sprintf(sequence, "%lld", quote.seq_no);
    //msd->set_msg_seq(sequence);
    // 卖盘
    cout << "msd3, origin: " << quote.symbol << ", "
         << "ask: " << quote.asks.size() << ", "
         << "bid: " << quote.bids.size() << " "
         << endl;

    for( auto iter = quote.asks.begin() ; iter != quote.asks.end() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
        {
            continue;
        }
            
        DepthWithDecimal* depth = msd->add_asks();
        set_decimal(depth->mutable_price(), iter->first);
        set_decimal(depth->mutable_volume(), iter->second.total_volume);
    }
    for( auto iter = quote.bids.rbegin() ; iter != quote.bids.rend() ; iter ++) {
        if( check_total_volume && iter->second.total_volume.is_zero() )
        {
            continue;
        }
            
        DepthWithDecimal* depth = msd->add_bids();
        set_decimal(depth->mutable_price(), iter->first);
        set_decimal(depth->mutable_volume(), iter->second.total_volume);
    }
    cout << "msd3, transd:" << quote.symbol << ", ask: " << msd->asks_size() << ", bid: " << msd->bids_size() << endl;

    //if (msd->asks_size() != quote.asks.size())
    //{
    //    cout << "*** ask 0 volumne count: " << quote.asks.size() - msd->asks_size() << endl;
    //}
    //if (msd->bids_size() != quote.bids.size())
    //{
    //    cout << "*** bid 0 volumne count: " << quote.bids.size() - msd->bids_size() << endl;
    //}    
}
