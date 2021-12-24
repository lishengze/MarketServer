#include "depth_aggregater.h"

#include "stream_engine_config.h"

#include "depth_aggregater.h"

#include "Log/log.h"
#include "util/tool.h"

/////////////////////////////////////////////////////////////////////
DepthAggregater::DepthAggregater():thread_run_(true)
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
            std::unique_lock<std::mutex> l{ mutex_config_ };
            for( const auto& cfg : configs_ )
            {   
                if (is_data_too_fast_or_init(cfg.first, cfg.second.frequency)) 
                {
                    LOG_INFO(cfg.first + " too fast, " + std::to_string(cfg.second.frequency));
                    continue;
                }
                calculate_symbols.push_back(make_pair(cfg.first, cfg.second));                
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

bool DepthAggregater::is_data_too_fast_or_init(TSymbol symbol, int standard_fre)
{
    try
    {
        type_tick now = get_miliseconds();
        if( standard_fre == 0 ) standard_fre = 1;
        auto iter = last_clocks_.find(symbol);         

        if (iter == last_clocks_.end())
        {
            last_clocks_[symbol] = now;
            return true;
        }   

        if( (now - iter->second) < (1000/standard_fre) )
        {
            return true;
        }

        last_clocks_[symbol] = now;
        return false;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return true;
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

    LOG_INFO("_calc_symbol: " +  symbol);

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

            // if (quote.symbol == "BTC_USDT")
            // {
            //     LOG_INFO(quote.str());
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

        if (snap.symbol == "BTC_USDT") LOG_INFO("Output " + snap.str());
        p_comm_->publish_depth(snap);
    }
}

void DepthAggregater::on_snap( SDepthQuote& quote)
{
    try
    {
        std::unique_lock<std::mutex> l{ mutex_quotes_ };

        if (quote.symbol == "BTC_USDT") LOG_INFO("Input " + quote.str());

        quotes_[quote.symbol][quote.exchange] = quote;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void DepthAggregater::set_config(unordered_map<TSymbol, SMixerConfig> & new_config)
{
    std::unique_lock<std::mutex> l{ mutex_config_ };
    
    configs_ = new_config;
}

