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

void MixCalculator::init(const set<TSymbol>& symbols, const set<TExchange>& exchanges)
{
    for( const auto& symbol : symbols ) {
        for( const auto& exchange : exchanges ) {
            caches_[symbol][exchange] = new CalcCache();
        }
    }
}

bool MixCalculator::add_kline(const TExchange& exchange, const TSymbol& symbol, const vector<KlineData>& input, vector<KlineData>& output)
{
    auto iter_symbol_cache = caches_.find(symbol);
    if( iter_symbol_cache == caches_.end() )
        return false;
    unordered_map<TExchange, CalcCache*>& symbol_cache = iter_symbol_cache->second;
    auto iter_exchange_cache = symbol_cache.find(exchange);
    if( iter_exchange_cache == symbol_cache.end() )
        return false;
    CalcCache* cache = iter_exchange_cache->second;

    // 计算除了自己之外最小可计算位置
    type_tick tail_min = INVALID_INDEX;
    for( const auto& cache : symbol_cache ) {
        if( cache.first == exchange )
            continue;
        type_tick tail = cache.second->get_last_index();
        if( tail == INVALID_INDEX ) {
            tail_min = INVALID_INDEX;
            break;
        }
        if( tail < tail_min ) {
            tail_min = tail;
        }
    }

    // 
    for( const auto& kline : input ) {
        type_tick last_index = cache->get_last_index();
        if( last_index == INVALID_INDEX || kline.index > last_index ) {
            cache->klines.push_back(kline);
        } else if ( last_index == kline.index ) {
            cache->klines.back() = kline;
        } else {
            // 倒着走
            return false;
        }

        if( tail_min != INVALID_INDEX && kline.index <= tail_min ) {
            // 触发计算&清理数据 时间戳=kline.index
            vector<KlineData> datas;
            for( auto& cache : symbol_cache ) {
                KlineData tmp = cache.second->get_index(kline.index);
                if( tmp.volume < VOLUME_PRECISE )
                    continue;
                datas.push_back(tmp);
            }
            KlineData mixed_kline = calc_mixed_kline(kline.index, datas);
            output.push_back(mixed_kline);
        }
    }

    return true;
}


//////////////////////////////////////////////////////////////////
KlineMixer::KlineMixer()
{

}

KlineMixer::~KlineMixer()
{

}

void KlineMixer::start()
{
    set<TExchange> exchanges;
    /*for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
        exchanges.insert(*iterExchange);
    }*/
    set<TSymbol> symbols;
    /*
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        symbols.insert(*iterSymbol);
    }*/
    min1_kline_calculator_.init(symbols, exchanges);
    min60_kline_calculator_.init(symbols, exchanges);
}

void KlineMixer::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline)
{
    if( resolution == 60 ) {
        vector<KlineData> output;
        min1_kline_calculator_.add_kline(exchange, symbol, kline, output);
        for( const auto& v : callbacks_) {
            v->on_kline(symbol, 60, output);
        }
    } else if( resolution == 3600 ) {
        vector<KlineData> output;
        min60_kline_calculator_.add_kline(exchange, symbol, kline, output);
        for( const auto& v : callbacks_) {
            v->on_kline(symbol, 3600, output);
        }
    }
}

bool KlineMixer::get_kline(const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    return db_interface_->get_kline(symbol, resolution, start_time, end_time, klines);
}