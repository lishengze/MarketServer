#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"
#include "stream_engine_config.h"

using FuncAddDepth = std::function<DepthLevel*()>;
using FuncAddDepth2 = std::function<Depth*()>;

inline void mixquote_to_pbquote_depth(const SMixDepthPrice* depths, FuncAddDepth func)
{
    int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        DepthLevel* depth = func();
        depth->mutable_price()->set_value(ptr->price.value);
        depth->mutable_price()->set_base(ptr->price.base);
        for(auto &v : ptr->volume) {
            const double volume = v.second;
            depth->set_volume(volume);
        }
        depth_count += 1;
        ptr = ptr->next;
    }
}

inline std::shared_ptr<QuoteData> mixquote_to_pbquote(const string& exchange, const string& symbol, const SMixQuote& quote) {
    std::shared_ptr<QuoteData> msd = std::make_shared<QuoteData>();
    msd->set_symbol(symbol);
    msd->set_exchange(exchange);
    msd->set_msg_seq(quote.sequence_no);

    // 卖盘
    FuncAddDepth f1 = std::bind(&QuoteData::add_ask_depth, msd);
    mixquote_to_pbquote_depth(quote.asks, f1);
    // 买盘
    FuncAddDepth f2 = std::bind(&QuoteData::add_bid_depth, msd);
    mixquote_to_pbquote_depth(quote.bids, f2);

    return msd;
};

inline void mixquote_to_pbquote2_depth(const SMixDepthPrice* depths, FuncAddDepth2 func, bool is_ask)
{
    int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        Depth* depth = func();
        depth->set_price(ptr->price.get_str_value());
        for(auto &v : ptr->volume) {
            (*depth->mutable_data())[v.first] = v.second;
        }
        depth_count += 1;
        ptr = ptr->next;
    }
}

inline std::shared_ptr<MarketStreamData> mixquote_to_pbquote2(const string& symbol, const SMixQuote* src)
{
    std::shared_ptr<MarketStreamData> msd = std::make_shared<MarketStreamData>();
    msd->set_symbol(symbol);

    // 卖盘
    FuncAddDepth2 f1 = std::bind(&MarketStreamData::add_ask_depths, msd);
    mixquote_to_pbquote2_depth(src->asks, f1, true);
    // 买盘
    FuncAddDepth2 f2 = std::bind(&MarketStreamData::add_bid_depths, msd);
    mixquote_to_pbquote2_depth(src->bids, f2, false);

    return msd;
};

inline void process_precise_depth(const SDepthPrice* src, unsigned int src_length, const SymbolFee& fee, int precise, SDepthPrice* dst, unsigned int& dst_length, bool is_ask)
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

inline void process_precise(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    // 精度统一
    int precise = CONFIG->get_precise(symbol);
    // 考虑手续费因素
    SymbolFee fee = CONFIG->get_fee(exchange, symbol);
    
    vassign(dst.exchange, MAX_EXCHANGE_NAME_LENGTH, src.exchange);
    vassign(dst.symbol, MAX_SYMBOL_NAME_LENGTH, src.symbol);
    vassign(dst.sequence_no, src.sequence_no);
    //vassign(dst.time_arrive, src.time_arrive);

    process_precise_depth(src.asks, src.ask_length, fee, precise, dst.asks, dst.ask_length, true);
    process_precise_depth(src.bids, src.bid_length, fee, precise, dst.bids, dst.bid_length, false);
};