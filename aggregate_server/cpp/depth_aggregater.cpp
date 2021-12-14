#include "depth_aggregater.h"

#include "stream_engine_config.h"

#include "depth_aggregater.h"

#include "Log/log.h"
#include "util/tool.h"

/////////////////////////////////////////////////////////////////////
DepthAggregater::DepthAggregater(QuoteSourceCallbackInterface* engine)
: engine_{engine}, thread_run_(true)
{

}

DepthAggregater::~DepthAggregater()
{
    thread_run_ = false;
    if (thread_loop_) {
        if (thread_loop_->joinable()) {
            thread_loop_->join();
        }
        delete thread_loop_;
    }
}


void DepthAggregater::start()
{
    if( thread_loop_ )
        return;
    thread_loop_ = new std::thread(&DepthAggregater::_thread_loop, this);
}

void DepthAggregater::_thread_loop()
{
    unordered_map<TSymbol, type_seqno> sequences;
    LOG_INFO("\n_thread_loop Start ");
    while( thread_run_ ) 
    {
        vector<pair<TSymbol, SMixerConfig>> calculate_symbols;
        {
            type_tick now = get_miliseconds();
            std::unique_lock<std::mutex> l{ mutex_config_ };
            for( const auto& cfg : configs_ )
            {      
                const TSymbol& symbol = cfg.first;    
                float frequency = cfg.second.frequency;
                if( frequency == 0 )
                    frequency = 1;
                auto iter = last_clocks_.find(symbol);                
                if( iter != last_clocks_.end() && (now - iter->second) < (1000/frequency) )
                {
                    continue;
                }
                calculate_symbols.push_back(make_pair(symbol, cfg.second));
                last_clocks_[symbol] = now;
            }
        }
        
        // 逐个计算
        for( const auto& symbol : calculate_symbols ) {
            sequences[symbol.first] ++;
            _calc_symbol(symbol.first, symbol.second, sequences[symbol.first]);
        }
        
        // 休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(CONFIG->depth_compute_millsecs));
    }
}

void mix_quote(map<SDecimal, SDepth>& dst, const map<SDecimal, SDepth>& src, const TExchange& exchange, const SMixerConfig& config, bool is_ask)
{    
    SymbolFee fee;
    auto iter = config.fees.find(exchange);
    if( iter != config.fees.end() ) {
        fee = iter->second;
    }

    for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
    {
        SDecimal scaledPrice;
        fee.compute(iter->first, scaledPrice, is_ask);
        scaledPrice.scale(config.precise, is_ask);
        dst[scaledPrice].volume_by_exchanges[exchange] = iter->second.volume;
    }
}

void normalize(map<SDecimal, SDepth>& src, const SMixerConfig& config)
{
    SDecimal volume = 0;
    for( auto iter = src.begin() ; iter != src.end() ; iter++ ) 
    {
        volume = 0;
        for( auto& v : iter->second.volume_by_exchanges ) {
            v.second.scale(config.vprecise);
            volume += v.second;
        }
        iter->second.volume = volume;
    }
}

void DepthAggregater::_calc_symbol(const TSymbol& symbol, const SMixerConfig& config, type_seqno seqno)
{
    // 行情
    SDepthQuote snap;
    snap.sequence_no = seqno;
    snap.origin_time = 0;
    snap.arrive_time = get_miliseconds();
    snap.price_precise = config.precise;
    snap.volume_precise = config.vprecise;
    snap.amount_precise = config.aprecise;
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };
        for( const auto& data : quotes_[symbol] ) 
        {
            const TExchange& exchange = data.first;
            const SDepthQuote& ori_quote = data.second;


            SDepthQuote& quote = const_cast<SDepthQuote&>(ori_quote);

            // if (filter_zero_volume(quote))
            // {
            //     LOG_WARN( "_calc_symbol " + exchange + "." + quote.symbol + ", ask.size: " + std::to_string(quote.asks.size())
            //             + ", bid.size: " + std::to_string(quote.bids.size()));  
            //     continue;
            // }

            if( quote.origin_time > snap.origin_time ) // 交易所时间取聚合品种中较大的
                snap.origin_time = quote.origin_time;
            mix_quote(snap.asks, quote.asks, exchange, config, true);
            mix_quote(snap.bids, quote.bids, exchange, config, false);
        }
    }

    normalize(snap.asks, config);
    normalize(snap.bids, config);

    if( snap.origin_time > 0 ) {
        snap.symbol = symbol;
        snap.exchange = MIX_EXCHANGE_NAME;        
        engine_->on_snap(snap);
    }
}

void DepthAggregater::on_snap(const TExchange& exchange, const TSymbol& symbol, const SDepthQuote& quote) 
{
    std::unique_lock<std::mutex> l{ mutex_quotes_ };

    quotes_[symbol][exchange] = quote;
}

void DepthAggregater::set_config(unordered_map<TSymbol, SMixerConfig> & new_config)
{
    std::unique_lock<std::mutex> l{ mutex_config_ };
    
    configs_ = new_config;
}

