#include "kline_processor.h"
#include "Log/log.h"
#include "util/tool.h"

KlineData calc_mixed_kline(type_tick index, const vector<KlineData>& datas) 
{
    KlineData ret;
    ret.index = index;
    ret.volume = 0;
    for( const auto& v : datas ) {
        ret.volume = ret.volume + v.volume;
    }
    for( const auto& v : datas ) {
        ret.px_open = ret.px_open + v.px_open * v.volume.get_value();
        ret.px_high = ret.px_high + v.px_high * v.volume.get_value();
        ret.px_low = ret.px_low + v.px_low * v.volume.get_value();
        ret.px_close = ret.px_close + v.px_close * v.volume.get_value();
    }
    ret.px_open = ret.px_open / ret.volume.get_value();
    ret.px_low = ret.px_low / ret.volume.get_value();
    ret.px_high = ret.px_high / ret.volume.get_value();
    ret.px_close = ret.px_close / ret.volume.get_value();
    return ret;
}

KlineAggregater::KlineAggregater()
{

}

KlineAggregater::~KlineAggregater()
{

}

void KlineAggregater::set_meta(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };

    auto iter = caches_.find(symbol);
    if( iter == caches_.end() ) {
        for( const auto& exchange : exchanges ) {
            // std::cout << "\nMixCalculator::set_symbol " << symbol << " " << exchange << std::endl;
            caches_[symbol][exchange] = new CalcCache();
        }
        return;
    }

    unordered_map<TExchange, CalcCache*>& symbol_cache = iter->second;
    for( const auto& exchange : exchanges ) 
    {
        auto iter2 = symbol_cache.find(exchange);
        if( iter2 == symbol_cache.end() ) {
            caches_[symbol][exchange] = new CalcCache();
        }
    }

    for(auto iter3 = symbol_cache.begin() ; iter3 != symbol_cache.end() ;)
    {
        const TExchange& exchange = iter3->first;
        auto iter2 = exchanges.find(exchange);
        if( iter2 == exchanges.end() ) {
            symbol_cache.erase(iter3++);
        } else {
            iter3++;
        }
    }
}

void KlineAggregater::set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& meta_map)
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };

        std::unordered_map<TSymbol, std::set<TExchange>> added_meta;
        std::unordered_map<TSymbol, std::set<TExchange>> removed_meta;

        get_delata_meta(meta_map, added_meta, removed_meta);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void KlineAggregater::get_delata_meta(const std::unordered_map<TSymbol, std::set<TExchange>>& new_meta_map,
                    std::unordered_map<TSymbol, std::set<TExchange>>& added_meta,
                    std::unordered_map<TSymbol, std::set<TExchange>>& removed_meta)
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };

        for (auto iter:new_meta_map)
        {
            const string& symbol = iter.first;
            const std::set<TExchange>& exchange_set = iter.second;

            if (caches_.find(symbol) == caches_.end())
            {
                added_meta[symbol] = exchange_set;
            }
            else
            {
                for (auto exchange:exchange_set)
                {
                    if (caches_[symbol].find(exchange) == caches_[symbol].end())
                    {
                        added_meta[symbol].emplace(exchange);
                    }
                }                
            }
        }

        for (auto iter:caches_)
        {
            const string& symbol = iter.first;
            const unordered_map<TExchange, CalcCache*>& exchange_map = iter.second;            

            if (new_meta_map.find(symbol) == new_meta_map.end())
            {
                for (auto iter2:exchange_map)
                {
                    removed_meta[symbol].emplace(iter2.first);
                }
            }
            else
            {
                for (auto iter2:exchange_map)
                {
                    if (new_meta_map[symbol].find(iter2.first) == new_meta_map[symbol].end())
                    {
                        removed_meta[symbol].emplace(iter2.first);
                    }                    
                }                
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}                    

bool KlineAggregater::add_kline(const KlineData& input, KlineData& output)
{
    try
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };
        
        // 寻找对应的cache
        auto iter_symbol_cache = caches_.find(input.symbol);
        if( iter_symbol_cache == caches_.end() )
        {
            LOG_WARN("caches can not find " + input.exchange + " " + input.symbol);

            return false;
        }
            
        unordered_map<TExchange, CalcCache*>& symbol_cache = iter_symbol_cache->second;
        auto iter_exchange_cache = symbol_cache.find(input.exchange);
        if( iter_exchange_cache == symbol_cache.end() )
        {
            LOG_WARN("symbol_cache can not find " + input.exchange + " " + input.symbol);

            return false;
        }
            
        CalcCache* cur_symbol_exchange_cache = iter_exchange_cache->second;       

        // 缓存最新K线
        type_tick last_index = cur_symbol_exchange_cache->get_last_index();
        if( last_index == INVALID_INDEX || input.index > last_index ) {
            cur_symbol_exchange_cache->klines.push_back(input);
        } else if ( last_index == input.index ) {
            cur_symbol_exchange_cache->klines.back() = input;
        } else {
            LOG_WARN(input.symbol + " kline go back. last_index: " + std::to_string(last_index) + ", new_index: " + std::to_string(input.index));
            return false;
        }

        // 更新了时间为 kline.index 的K线
        // 1. 检查是否可以计算，计算条件：所有市场时间>=kline.index
        // 2. 如果可以计算，则清除所有市场时间<kline.index的数据
        
        bool can_calculate = true;
        bool found = false;

        vector<KlineData> datas;
        for( const auto& symbol_cache_iter : symbol_cache ) 
        {
            const TExchange& exchange = symbol_cache_iter.first;
            const CalcCache* symbol_exchange_cache = symbol_cache_iter.second;

            // bool found = false;
            for( const auto& cur_exchange_kline : symbol_exchange_cache->klines ) 
            {
                if( cur_exchange_kline.index == input.index ) 
                {
                    found = true;   // QS: 只要有一个交易所的有和当前 k 线时间一致的 数据，就会进行计算；有逻辑错误，应该是所有交易所都有当前k线时间数据；
                    datas.push_back(cur_exchange_kline);
                }
            }

        }

        if (datas.size() > 0)
        {
            output = calc_mixed_kline(input.index, datas);
            output.exchange = MIX_EXCHANGE_NAME;
            output.symbol = input.symbol;

            // 开始清理
            for( auto& cache : symbol_cache ) 
            {
                CalcCache* c = cache.second;
                c->clear_earlier_kline(input.index);
            }
        }         

        return is_kline_valid(output);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    return false;
}

KlineProcessor::~KlineProcessor()
{

}

void KlineProcessor::set_meta(const std::unordered_map<TSymbol, std::set<TExchange>>&meta_map)
{
    try
    {
        // min1_kline_aggregator_.set_meta(symbol, exchanges);

        // min60_kline_aggregator_.set_meta(symbol, exchanges);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void KlineProcessor::process(KlineData& src)
{
    try
    {
        KlineData output;
        if (min1_kline_aggregator_.add_kline(src, output))
        {
            engine_->on_kline(output);
        }
        else
        {

        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void KlineProcessor::set_config(unordered_map<TSymbol, SMixerConfig>& symbol_config)
{
    try
    {
       min1_kline_aggregator_.set_config(symbol_config);

    //    min60_kline_aggregator_.set_config(symbol_config);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}