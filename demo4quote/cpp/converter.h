#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"
#include "stream_engine_config.h"

using FuncAddDepth2 = std::function<DepthWithDecimal*()>;

inline void mixquote_to_pbquote2_depth(const SMixDepthPrice* depths, FuncAddDepth2 func, type_uint32 depth, bool is_ask)
{
    unsigned int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < depth ) {
        DepthWithDecimal* depth = func();
        set_decimal(depth->mutable_price(), ptr->price);
        SDecimal total_volume = 0;
        for(auto &v : ptr->volume) {
            Decimal tmp;
            set_decimal(&tmp, v.second);
            (*depth->mutable_data())[v.first] = tmp;
        }        
        set_decimal(depth->mutable_volume(), total_volume);
        depth_count += 1;
        ptr = ptr->next;
    }
}

inline std::shared_ptr<MarketStreamDataWithDecimal> mixquote_to_pbquote2(const string& exchange, const string& symbol, const SMixQuote* src, type_uint32 depth, bool is_snap)
{
    std::shared_ptr<MarketStreamDataWithDecimal> msd = std::make_shared<MarketStreamDataWithDecimal>();
    msd->set_exchange(exchange);
    msd->set_symbol(symbol);
    msd->set_seq_no(src->sequence_no);
    msd->set_is_snap(is_snap);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamDataWithDecimal::add_asks, msd);
    mixquote_to_pbquote2_depth(src->asks, f1, depth, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamDataWithDecimal::add_bids, msd);
    mixquote_to_pbquote2_depth(src->bids, f2, depth, false);

    return msd;
};

inline void depth_to_pbquote2_depth(const string& exchange, const string& symbol, const map<SDecimal, SDecimal>& depths, FuncAddDepth2 func, type_uint32 depth, bool is_ask)
{
    type_uint32 count = 0;
    if( is_ask ) {
        for( auto iter = depths.begin() ; iter != depths.end() && count < depth ; iter ++, count ++) {
            DepthWithDecimal* depth = func();
            set_decimal(depth->mutable_price(), iter->first);
            set_decimal(depth->mutable_volume(), iter->second);
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() && count < depth ; iter ++, count ++) {
            DepthWithDecimal* depth = func();
            set_decimal(depth->mutable_price(), iter->first);
            set_decimal(depth->mutable_volume(), iter->second);
        }
    }
}

inline std::shared_ptr<MarketStreamDataWithDecimal> depth_to_pbquote2(const string& exchange, const string& symbol, const SDepthQuote& src, type_uint32 depth, bool is_snap)
{
    std::shared_ptr<MarketStreamDataWithDecimal> msd = std::make_shared<MarketStreamDataWithDecimal>();
    msd->set_exchange(exchange);
    msd->set_symbol(symbol);
    msd->set_time(src.arrive_time);
    msd->set_seq_no(src.sequence_no);
    msd->set_is_snap(is_snap);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamDataWithDecimal::add_asks, msd);
    depth_to_pbquote2_depth(exchange, symbol, src.asks, f1, depth, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamDataWithDecimal::add_bids, msd);
    depth_to_pbquote2_depth(exchange, symbol, src.bids, f2, depth, false);

    return msd;
};

inline void process_precise_mixdepth(SMixDepthPrice* dst, int precise, const SMixDepthPrice* src, bool is_ask)
{
    SMixDepthPrice* current = dst;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    while( src != NULL ) {        
        const SDecimal& price = src->price;
        const unordered_map<TExchange, SDecimal>& volume = src->volume;

        SDecimal scaledPrice;
        scaledPrice.from(price, precise, is_ask);
        
        bool is_new_price = is_ask ? (scaledPrice > lastPrice) : (scaledPrice < lastPrice);
        if( is_new_price ) {
            SMixDepthPrice* tmp = new SMixDepthPrice();
            tmp->next = NULL;
            tmp->price = scaledPrice;
            tmp->volume = volume;
            if( current == NULL ) {
                current = tmp;
            } else {
                current->next = tmp;
                current = tmp;
            }
        }
        // 累积到current
        for( const auto& v : volume ) {
            current->volume[v.first] = current->volume[v.first] + v.second;
        }

        src = src->next;
    }
}