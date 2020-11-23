#include "kline_database.h"

KlineDatabase::KlineDatabase()
{

}

KlineDatabase::~KlineDatabase()
{

}

void KlineDatabase::start()
{

}

void KlineDatabase::on_kline(const TSymbol& symbol, int resolution, const vector<KlineData>& kline)
{

}

bool KlineDatabase::get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& kline)
{
    return true;
}
