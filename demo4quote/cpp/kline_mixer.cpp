#include "stream_engine_config.h"
#include "kline_mixer.h"


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

MixCalculator::MixCalculator()
{

}

MixCalculator::~MixCalculator()
{

}

void MixCalculator::set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };

    auto iter = caches_.find(symbol);
    if( iter == caches_.end() ) {
        for( const auto& exchange : exchanges ) {
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

bool MixCalculator::add_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& input, vector<KlineData>& output)
{
    output.clear();
    
    std::unique_lock<std::mutex> inner_lock{ mutex_cache_ };
    
    // 寻找对应的cache
    auto iter_symbol_cache = caches_.find(symbol);
    if( iter_symbol_cache == caches_.end() )
        return false;
    unordered_map<TExchange, CalcCache*>& symbol_cache = iter_symbol_cache->second;
    auto iter_exchange_cache = symbol_cache.find(exchange);
    if( iter_exchange_cache == symbol_cache.end() )
        return false;
    CalcCache* cache = iter_exchange_cache->second;

    // 
    for( const auto& kline : input ) {
        // 缓存最新K线
        type_tick last_index = cache->get_last_index();
        if( last_index == INVALID_INDEX || kline.index > last_index ) {
            cache->klines.push_back(kline);
        } else if ( last_index == kline.index ) {
            cache->klines.back() = kline;
        } else {
            // 倒着走
            tfm::printfln("%s kline go back. last_index=%lu, new_index=%lu", symbol.c_str(), last_index, kline.index);
            return false;
        }

        // 更新了时间为 kline.index 的K线
        // 1. 检查是否可以计算，计算条件：所有市场时间>=kline.index
        // 2. 如果可以计算，则清除所有市场时间<kline.index的数据
        bool can_calculate = true;
        vector<KlineData> datas;
        for( const auto& cache : symbol_cache ) 
        {
            const TExchange& exchange = cache.first;
            const CalcCache* c = cache.second;

            bool found = false;
            for( const auto& v : c->klines ) {
                if( v.index == kline.index ) {
                    found = true;
                    datas.push_back(v);
                }
            }
            if( !found ) {
                tfm::printfln("%s wait for exchange %s", symbol.c_str(), exchange.c_str());
                can_calculate = false;
                break;
            }
        }
        if( !can_calculate ){
            continue;
        }

        // 开始计算
        KlineData mixed_kline = calc_mixed_kline(kline.index, datas);
        output.push_back(mixed_kline);

        // 开始清理
        for( auto& cache : symbol_cache ) 
        {
            CalcCache* c = cache.second;
            c->clear(kline.index);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////
KlineCache::KlineCache()
{

}

KlineCache::~KlineCache()
{

}

void KlineCache::_shorten(vector<KlineData>& klines)
{
    if( klines.size() > limit_ * 2 ) {
        klines.erase(klines.begin(), klines.end() - limit_);
    }
}

void KlineCache::fill_klines(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_data_ };
    cache = data_;
}

bool is_same_interval(type_tick index1, type_tick index2, int interval){
    return index1/interval == index2/interval;
}
void KlineCache::update_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& klines, vector<KlineData>& outputs, vector<KlineData>& output_60mins)
{
    // 锁住对象
    std::unique_lock<std::mutex> inner_lock{ mutex_data_ };
    vector<KlineData>& dst = data_[exchange][symbol];

    // 填补跳空数据
    outputs = klines;
    if( dst.size() > 0 && outputs.size() > 0 ) {
        const KlineData& last = dst.back();
        const KlineData& first = klines.front();
        vector<KlineData> patch;
        type_tick fix_index = last.index + this->resolution_;
        while( fix_index < first.index ) {
            tfm::printfln("%s.%s kline patch index=%u to %u", exchange, symbol, fix_index, first.index);
            KlineData tmp = last;
            tmp.index = fix_index;
            tmp.volume = 0;
            patch.push_back(tmp);
            fix_index += this->resolution_;
        }
        outputs.insert(outputs.begin(), patch.begin(), patch.end());
    }
    
    // 合并到内存
    if( outputs.size() <= 10 )
    {
        // 数量少的时候，直接插入。为什么是10？
        for( auto iter = outputs.begin() ; iter != outputs.end() ; iter++ )
        {
            const KlineData& kline = *iter;
            bool inserted = false;
            for( auto iter2 = dst.begin() ; iter2 != dst.end() ; iter2++ )
            {
                if( kline.index < iter2->index ) {
                    dst.insert(iter2, kline);
                    inserted = true;
                    break;
                } else if( kline.index == iter2->index ) {
                   *iter2 = kline; 
                   inserted = true;
                   break;
                }
            }
            if( !inserted ) {
                dst.push_back(kline);
            }
        }
    }
    else
    {
        // 转为map处理
        map<type_tick, KlineData> tmp;
        for( auto iter = dst.begin() ; iter != dst.end() ; iter++ ){
            tmp[iter->index] = *iter;
        }
        for( auto iter = outputs.begin() ; iter != outputs.end() ; iter++ ){
            tmp[iter->index] = *iter;
        }
        dst.clear();
        for( const auto& v : tmp ) {
            //tfm::printfln("%s.%s %d", exchange, symbol, v.second.index);
            dst.push_back(v.second);
        }
    }

    // 根据1分钟计算60分钟周期K线
    if( resolution_ == 60 ) 
    {
        output_60mins.clear();
        vector<KlineData>& dst_60min = data_60min_[exchange][symbol];
        
        // 更新到60分钟K线
        for( auto iter = outputs.begin() ; iter != outputs.end() ; iter++ )
        {
            const KlineData& kline = *iter;
            // 非整点直接过滤
            if( dst_60min.size() == 0 && kline.index % 3600 != 0 ) { 
                continue;
            }
            if( dst_60min.size() > 0 && is_same_interval(dst_60min.back().index, kline.index, 3600) ) {
                // 更新到最后一根
                KlineData& dst = dst_60min.back();
                if( dst.px_high < kline.px_high ) {
                    dst.px_high = kline.px_high;                    
                }
                if( dst.px_low > kline.px_low ) {
                    dst.px_low = kline.px_low;
                }
                dst.px_close = kline.px_close;
                dst.volume += kline.volume;
                if( output_60mins.size() == 0 || output_60mins.back().index < dst.index ) {
                    output_60mins.push_back(dst);
                } else {
                    output_60mins.back() = dst;
                }
            } else if( dst_60min.size() == 0 || (kline.index > dst_60min.back().index) ) {
                // 新增一根
                KlineData tmp = kline;
                tmp.index = kline.index / 3600 * 3600;
                dst_60min.push_back(tmp);
                output_60mins.push_back(tmp);
            } else {
                // 倒退，直接丢弃
            }
        }
    }

    _shorten(dst);
}

//////////////////////////////////////////////////////////////////
KlineMixer::KlineMixer()
{
}

KlineMixer::~KlineMixer()
{

}

void KlineMixer::set_symbol(const TSymbol& symbol, const unordered_set<TExchange>& exchanges)
{
    min1_kline_calculator_.set_symbol(symbol, exchanges);
    min60_kline_calculator_.set_symbol(symbol, exchanges);
}

void KlineMixer::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline, bool is_init)
{
    vector<KlineData> output;
    switch( resolution ) 
    {
        case 60:
        {
            min1_kline_calculator_.add_kline(exchange, symbol, kline, output);
            break;
        }
        case 3600:
        {
            min60_kline_calculator_.add_kline(exchange, symbol, kline, output);
            break;
        }
        default:
        {
            _log_and_print("%s-%s unknown resolution %d", exchange.c_str(), symbol.c_str(), resolution);
            break;
        }
    }
    
    engine_interface_->on_kline(MIX_EXCHANGE_NAME, symbol, resolution, output, is_init);
}

//////////////////////////////////////////////////////////////////
KlineHubber::KlineHubber()
{
    min1_cache_.set_limit(KLINE_CACHE_MIN1);
    min1_cache_.set_resolution(60);
    min60_cache_.set_limit(KLINE_CACHE_MIN60);
    min60_cache_.set_resolution(3600);
}

KlineHubber::~KlineHubber()
{

}

void KlineHubber::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init, vector<KlineData>& outputs)
{
    vector<KlineData> output_60mins, nouse;
    // 写入cache
    switch( resolution ) 
    {
        case 60:
        {
            min1_cache_.update_kline(exchange, symbol, klines, outputs, output_60mins);        
            min60_cache_.update_kline(exchange, symbol, output_60mins, nouse, nouse);    
            break;
        }
        case 3600:
        {
            //min60_cache_.update_kline(exchange, symbol, klines, outputs);
            break;
        }
        default:
        {
            _log_and_print("%s-%s unknown resolution %d", exchange.c_str(), symbol.c_str(), resolution);
            break;
        }
    }

    // 写入db缓存区
    db_interface_->on_kline(exchange, symbol, resolution, outputs, is_init);
    db_interface_->on_kline(exchange, symbol, 3600, output_60mins, is_init);

    // 更新回调
    if( !is_init && outputs.size() > 0 )
    {
        for( const auto& v : callbacks_) {
            v->on_kline(exchange, symbol, resolution, outputs);
            v->on_kline(exchange, symbol, 3600, output_60mins);
        }
    }
}

// start_time 为0代表无效
// end_time 为0代表无效
bool KlineHubber::get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    switch( resolution )
    {
        case 60:
        {
            db_interface_->get_kline(exchange, symbol, resolution, start_time, end_time, klines);
            break;
        }
        case 3600:
        {
            db_interface_->get_kline(exchange, symbol, resolution, start_time, end_time, klines);
            break;
        }
        default:
        {
            return false;
        }
    }

    // 按照请求过滤
    if( start_time != 0 ) {
        auto iter = klines.begin();
        for( ; iter != klines.end() ; iter++ ) {
            if( iter->index >= start_time ) {
                break;
            }
        }
        klines.erase(klines.begin(), iter);
    }
    if( end_time != 0 ) {
        auto iter = klines.rbegin();
        for( ; iter != klines.rend() ; iter++ ) {
            if( iter->index <= end_time ) {
                break;
            }
        }
        klines.erase(iter.base(), klines.end());
    }
    return true;
}

void KlineHubber::fill_cache(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min1, unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache_min60)
{
    min1_cache_.fill_klines(cache_min1);
    min60_cache_.fill_klines(cache_min60);
}
