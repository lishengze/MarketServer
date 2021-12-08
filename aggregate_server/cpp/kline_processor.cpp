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

void KlineAggregater::set_config(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
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
KlineProcessor::KlineProcessor()
{

}

KlineProcessor::~KlineProcessor()
{

}

void KlineProcessor::set_config(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
{
    try
    {
        min1_kline_aggregator_.set_config(symbol, exchanges);

        // min60_kline_aggregator_.set_config(symbol, exchanges);
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