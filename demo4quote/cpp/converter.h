#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"
#include "stream_engine_config.h"

using FuncAddDepth = std::function<DepthLevel*()>;

inline void mixquote_to_pbquote_depth(const SMixDepthPrice* depths, FuncAddDepth func)
{
    int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        DepthLevel* depth = func();
        depth->mutable_price()->set_value(ptr->price.value);
        depth->mutable_price()->set_base(ptr->price.base);
        for(auto &v : ptr->volume) {
            const TExchange& exchange = v.first;
            const double volume = v.second;
            DepthVolume* depthVolume = depth->add_data();
            depthVolume->set_volume(volume);
            depthVolume->set_exchange(exchange);
        }
        depth_count += 1;
        ptr = ptr->next;
    }
}

inline std::shared_ptr<QuoteData> mixquote_to_pbquote(const string& exchange, const string& symbol, const SMixQuote& quote) {
    std::shared_ptr<QuoteData> msd = std::make_shared<QuoteData>();
    msd->set_symbol(symbol);

    // 卖盘
    FuncAddDepth f1 = std::bind(&QuoteData::add_ask_depth, msd);
    mixquote_to_pbquote_depth(quote.asks, f1);
    // 买盘
    FuncAddDepth f2 = std::bind(&QuoteData::add_bid_depth, msd);
    mixquote_to_pbquote_depth(quote.bids, f2);

    return msd;
};

inline void mixquote_to_pbquote2_depth(const SMixDepthPrice* depths, const SDecimal& watermark, FuncAddDepth func, bool is_ask)
{
    bool patched =  false;
    unordered_map<TExchange, double> patched_volumes;
    int depth_count = 0;
    const SMixDepthPrice* ptr = depths;
    while( ptr != NULL && depth_count < CONFIG->grpc_publish_depth_ ) {
        if( is_ask ? (ptr->price > watermark) : (ptr->price < watermark) ) {
            DepthLevel* depth = func();
            depth->mutable_price()->set_value(ptr->price.value);
            depth->mutable_price()->set_base(ptr->price.base);
            if( !patched ) {
                patched = true;
                for(auto &v : ptr->volume) {
                    const TExchange& exchange = v.first;
                    const double& volume = v.second;
                    DepthVolume* depthVolume = depth->add_data();
                    depthVolume->set_volume(patched_volumes[exchange] + volume);
                    depthVolume->set_exchange(exchange);
                }
            } else {
                for(auto &v : ptr->volume) {
                    const TExchange& exchange = v.first;
                    const double& volume = v.second;
                    DepthVolume* depthVolume = depth->add_data();
                    depthVolume->set_volume(volume);
                    depthVolume->set_exchange(exchange);
                }
            }
            depth_count += 1;
        } else {
            for( auto &v : ptr->volume ) {
                const TExchange& exchange = v.first;
                const double& volume = v.second;
                patched_volumes[exchange] += volume;
            }
        }
        ptr = ptr->next;
    }
}

inline std::shared_ptr<QuoteData> mixquote_to_pbquote2(const string& symbol, const SMixQuote* src, const SDecimal& watermark)
{
    std::shared_ptr<QuoteData> msd = std::make_shared<QuoteData>();
    msd->set_symbol(symbol);

    // 卖盘
    FuncAddDepth f1 = std::bind(&QuoteData::add_ask_depth, msd);
    mixquote_to_pbquote2_depth(src->asks, watermark, f1, true);
    // 买盘
    FuncAddDepth f2 = std::bind(&QuoteData::add_bid_depth, msd);
    mixquote_to_pbquote2_depth(src->bids, watermark, f2, false);

    return msd;
};

inline void compress_quote_depth(const SDepthPrice* src, unsigned int src_length, int precise, SDepthPrice* dst, unsigned int& dst_length, bool is_ask)
{
    int count = 0;
    SDecimal lastPrice = is_ask ? SDecimal::min_decimal() : SDecimal::max_decimal();
    for (unsigned int i = 0 ; count < MAX_DEPTH && i < src_length; ++i )
    {
        const SDecimal& price = src[i].price;
        const double& volume = src[i].volume;

        // 卖价往上取整
        SDecimal scaledPrice;
        scaledPrice.from(price, precise, is_ask ? true : false); 

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

inline void compress_quote(const string& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    int precise = CONFIG->get_precise(symbol);
    
    vassign(dst.exchange, src.exchange);
    vassign(dst.symbol, src.symbol);
    vassign(dst.sequence_no, src.sequence_no);
    vassign(dst.time_arrive, src.time_arrive);

    compress_quote_depth(src.asks, src.ask_length, precise, dst.asks, dst.ask_length, true);
    compress_quote_depth(src.bids, src.bid_length, precise, dst.bids, dst.bid_length, false);
};
