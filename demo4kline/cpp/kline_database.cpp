#include "kline_database.h"
#include "kline_server_config.h"

KlineDatabase::KlineDatabase()
{

}

KlineDatabase::~KlineDatabase()
{

}

void KlineDatabase::start()
{

}

void KlineDatabase::on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& klines)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_caches_ };
    vector<KlineData>& data = caches_[symbol][resolution];
    for( const auto& v : klines ) {
        _log_and_print("%s index=%lu open=%s high=%s low=%s close=%s", symbol.c_str(), 
            v.index,
            v.px_open.get_str_value().c_str(),
            v.px_high.get_str_value().c_str(),
            v.px_low.get_str_value().c_str(),
            v.px_close.get_str_value().c_str()
            );

        if( data.size() == 0 || v.index > data.back().index ) {
            data.push_back(v);
        } else if( v.index == data.back().index ) {
            data.back() = v;
        } else {
            // 错误
        }
    }
}

bool KlineDatabase::get_kline(const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    std::unique_lock<std::mutex> inner_lock{ mutex_caches_ };
    klines = caches_[symbol][resolution];
    return true;
}
