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
        depth->set_price(ptr->price.get_raw());
        SDecimal total_volume = 0;
        for(auto &v : ptr->volume) {
            (*depth->mutable_data())[v.first] = v.second.get_raw();
            total_volume = total_volume + v.second;
        }        
        depth->set_volume(total_volume.get_raw());
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

inline void mixquote_to_pbquote3_depth(const SMixDepthPrice* depths, FuncAddDepth2 func, bool is_ask)
{
    /*
    unsigned int depth_count = 0;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    const SMixDepthPrice* ptr = depths;
    Depth* current_depth = NULL;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        const SDecimal& price = ptr->price;

        // 卖价往上取整
        SDecimal scaledPrice;
        fee.compute(price, scaledPrice, is_ask);
        scaledPrice.from(scaledPrice, precise, is_ask); 

        bool is_new_price = is_ask ? (scaledPrice > lastPrice) : (scaledPrice < lastPrice);
        if( is_new_price ) {
            depth_count ++;
            current_depth = func();
            current_depth->set_price(scaledPrice.get_str_value());
            lastPrice = scaledPrice;
        }
        double total_volume = current_depth->volume();
        for(auto &v : ptr->volume) {
            (*current_depth->mutable_data())[v.first] += v.second;
            total_volume += v.second;
        }        
        current_depth->set_volume(total_volume);
        ptr = ptr->next;
    }*/
}

inline std::shared_ptr<MarketStreamDataWithDecimal> mixquote_to_pbquote3(const string& exchange, const string& symbol, const SMixQuote* src, bool is_snap)
{
    // 精度统一
    //int precise = CONFIG->get_precise(symbol);
    // 考虑手续费因素
    //SymbolFee fee = CONFIG->get_fee(exchange, symbol);

    std::shared_ptr<MarketStreamDataWithDecimal> msd = std::make_shared<MarketStreamDataWithDecimal>();
    msd->set_exchange(exchange);
    msd->set_symbol(symbol);
    msd->set_seq_no(src->sequence_no);
    msd->set_is_snap(is_snap);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamDataWithDecimal::add_asks, msd);
    mixquote_to_pbquote3_depth(src->asks, f1, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamDataWithDecimal::add_bids, msd);
    mixquote_to_pbquote3_depth(src->bids, f2, false);

    return msd;
};

inline void depth_to_pbquote2_depth(const map<SDecimal, SDecimal>& depths, FuncAddDepth2 func, type_uint32 depth, bool is_ask)
{
    type_uint32 count = 0;
    if( is_ask ) {
        for( auto iter = depths.begin() ; iter != depths.end() && count < depth ; iter ++, count ++) {
            DepthWithDecimal* depth = func();
            cout << iter->first.get_str_value() << "\t" << iter->second.get_str_value();
            depth->set_price(iter->first.get_raw());
            depth->set_volume(iter->second.get_raw());
        }
    } else {
        for( auto iter = depths.rbegin() ; iter != depths.rend() && count < depth ; iter ++, count ++) {
            DepthWithDecimal* depth = func();
            depth->set_price(iter->first.data_.value_);
            depth->set_volume(iter->second.get_raw());
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
    depth_to_pbquote2_depth(src.asks, f1, depth, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamDataWithDecimal::add_bids, msd);
    depth_to_pbquote2_depth(src.bids, f2, depth, false);

    return msd;
};

/*
inline void process_precise_depth(const SDepthPrice* src, unsigned int src_length, int precise, const SymbolFee& fee, SDepthPrice* dst, unsigned int& dst_length, bool is_ask)
{
    int count = 0;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    for (unsigned int i = 0 ; count < MAX_DEPTH && i < src_length; ++i )
    {
        const SDecimal& price = src[i].price;
        const double& volume = src[i].volume;
        cout << price.get_str_value() << " " << volume << endl;

        // 卖价往上取整
        SDecimal scaledPrice;
        fee.compute(price, scaledPrice, is_ask);
        scaledPrice.from(scaledPrice, precise, is_ask); 

        bool is_new_price = is_ask ? (scaledPrice > lastPrice) : (scaledPrice < lastPrice);
        if( is_new_price ) {
            count ++;
            dst[count-1].price = scaledPrice;
            lastPrice = scaledPrice;
        }
        dst[count-1].volume += volume;
    }
    dst_length = count;
}
inline void process_precise(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& src, SDepthQuote& dst) 
{
    // 精度统一
    int precise = CONFIG->get_precise(symbol);
    // 考虑手续费因素
    SymbolFee fee = CONFIG->get_fee(exchange, symbol);
    
    vassign(dst.exchange, MAX_EXCHANGE_NAME_LENGTH, src.exchange);
    vassign(dst.symbol, MAX_SYMBOL_NAME_LENGTH, src.symbol);
    vassign(dst.sequence_no, src.sequence_no);
    //vassign(dst.time_arrive, src.time_arrive);

    process_precise_depth(src.asks, src.ask_length, precise, fee, dst.asks, dst.ask_length, true);
    process_precise_depth(src.bids, src.bid_length, precise, fee, dst.bids, dst.bid_length, false);
};
*/

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