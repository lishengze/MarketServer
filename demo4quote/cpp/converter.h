#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"
#include "stream_engine_config.h"

using FuncAddDepth2 = std::function<Depth*()>;

inline void mixquote_to_pbquote2_depth(const SMixDepthPrice* depths, FuncAddDepth2 func, bool is_ask)
{
    unsigned int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        Depth* depth = func();
        depth->set_price(ptr->price.get_str_value());
        double total_volume = 0;
        for(auto &v : ptr->volume) {
            (*depth->mutable_data())[v.first] = v.second;
            total_volume += v.second;
        }
        depth->set_volume(total_volume);
        depth_count += 1;
        ptr = ptr->next;
    }
}

inline std::shared_ptr<MarketStreamData> mixquote_to_pbquote2(const string& exchange, const string& symbol, const SMixQuote* src)
{
    std::shared_ptr<MarketStreamData> msd = std::make_shared<MarketStreamData>();
    msd->set_exchange(exchange);
    msd->set_symbol(symbol);
    msd->set_is_snap(true);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamData::add_asks, msd);
    mixquote_to_pbquote2_depth(src->asks, f1, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamData::add_bids, msd);
    mixquote_to_pbquote2_depth(src->bids, f2, false);

    return msd;
};

inline void depth_to_pbquote2_depth(const SDepthPrice* depths, unsigned int len, FuncAddDepth2 func, bool is_ask)
{
    for( unsigned int i = 0 ; i < len && i < CONFIG->grpc_publish_depth_ ; i++ ) {
        Depth* depth = func();
        depth->set_price(depths[i].price.get_str_value());
        depth->set_volume(depths[i].volume);
    }
}

inline std::shared_ptr<MarketStreamData> depth_to_pbquote2(const string& exchange, const string& symbol, const SDepthQuote& src)
{
    std::shared_ptr<MarketStreamData> msd = std::make_shared<MarketStreamData>();
    msd->set_exchange(exchange);
    msd->set_symbol(symbol);
    msd->set_is_snap(false);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamData::add_asks, msd);
    depth_to_pbquote2_depth(src.asks, src.ask_length, f1, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamData::add_bids, msd);
    depth_to_pbquote2_depth(src.bids, src.bid_length, f2, false);

    return msd;
};

inline void process_precise_depth(const SDepthPrice* src, unsigned int src_length, int precise, const SymbolFee& fee, SDepthPrice* dst, unsigned int& dst_length, bool is_ask)
{
    int count = 0;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    for (unsigned int i = 0 ; count < MAX_DEPTH && i < src_length; ++i )
    {
        const SDecimal& price = src[i].price;
        const double& volume = src[i].volume;

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

inline void process_precise_mixdepth(SMixDepthPrice* dst, int precise, const SMixDepthPrice* src, bool is_ask)
{
    SMixDepthPrice* current = dst;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    while( src != NULL ) {        
        const SDecimal& price = src->price;
        const unordered_map<TExchange, double>& volume = src->volume;

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
            current->volume[v.first] += v.second;
        }

        src = src->next;
    }
}

inline void compress_mixquote(const TSymbol& symbol, SMixQuote& dst, const SMixQuote& src)
{
    // 精度统一
    int precise = CONFIG->get_precise(symbol);
    
    vassign(dst.sequence_no, src.sequence_no);

    process_precise_mixdepth(dst.asks, precise, src.asks, true);
    process_precise_mixdepth(dst.bids, precise, src.bids, false);
}
