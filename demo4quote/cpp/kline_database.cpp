#include "stream_engine_config.h"
#include "kline_database.h"

string db_name = "klines.dat";

void _mix_klines(vector<KlineData>& dst, const vector<KlineData>& src)
{
    map<type_tick, KlineData> tmp;
    for( const auto& v : dst ) {
        tmp[v.index] = v;
    }
    for( const auto& v : src ) {
        tmp[v.index] = v;
    }
    dst.clear();
    for( const auto& v : tmp ) {
        dst.push_back(v.second);
    }
}

bool data_to_kline(const njson& data, KlineData& kline) {
    kline.index = int(data[0].get<double>());
    kline.px_open.from(data[1].get<string>());
    kline.px_high.from(data[2].get<string>());
    kline.px_low.from(data[3].get<string>());
    kline.px_close.from(data[4].get<string>());
    kline.volume.from(data[5].get<string>());
    return true;
}

njson kline_to_data( const KlineData& kline) {
    njson ret;
    ret.push_back(kline.index);
    ret.push_back(kline.px_open.get_str_value());
    ret.push_back(kline.px_high.get_str_value());
    ret.push_back(kline.px_low.get_str_value());
    ret.push_back(kline.px_close.get_str_value());
    ret.push_back(kline.volume.get_str_value());
    return ret;
}

string encode_klines(const vector<KlineData>& klines)
{
    njson data;
    for( const auto& v : klines ) {
        data.push_back(kline_to_data(v));
    }

    return data.dump();
}

bool decode_json_klines( const string& data, vector<KlineData>& klines)
{    
    klines.clear();

    njson js;    
    try
    {
        js = njson::parse(data);
    }
    catch(nlohmann::detail::exception& e)
    {
        _log_and_print("parse json fail %s", e.what());
        return false;
    }

    KlineData kline;
    for (auto iter = js.begin(); iter != js.end(); ++iter) {
        data_to_kline(*iter, kline);
        klines.push_back(kline);
    }
    return true;
}

KlineDatabase::Table::Table(SQLite::Database& db) 
{
    createTableKlineMin1(db);
    createTableKlineMin60(db);
}

void KlineDatabase::Table::createTableKlineMin1(SQLite::Database& db) {
    try
    {
        if (db.tableExists(KLINE_MIN1_TABLENAME)) {
            return; 
        }

        db.exec("CREATE TABLE " KLINE_MIN1_TABLENAME "(\
                Exchange TEXT, \
                Symbol TEXT, \
                Index INTEGER, \
                Data TEXT, \
                Ratio INTEGER, \
                CreateTime TEXT, \
                ModifyTime TEXT, \
            );");

        db.exec("CREATE INDEX " KLINE_MIN1_TABLENAME "_kline_index ON " KLINE_MIN1_TABLENAME "(Exchange, Symbol, Index);");
    }
    catch(const std::exception& e)
    {
        _log_and_print("[KlineDatabase] SQLite exception: %s", e.what());
    }
}

void KlineDatabase::Table::createTableKlineMin60(SQLite::Database& db) {
    try
    {
        if (db.tableExists(KLINE_MIN60_TABLENAME)) {
            return; 
        }

        db.exec("CREATE TABLE " KLINE_MIN60_TABLENAME "(\
                Exchange TEXT, \
                Symbol TEXT, \
                Index INTEGER, \
                Data TEXT, \
                Ratio INTEGER, \
                CreateTime TEXT, \
                ModifyTime TEXT, \
            );");

        db.exec("CREATE INDEX " KLINE_MIN60_TABLENAME "_kline_index ON " KLINE_MIN60_TABLENAME "(Exchange, Symbol, Index);");
    }
    catch(const std::exception& e)
    {
        _log_and_print("[KlineDatabase] SQLite exception: %s", e.what());
    }
}

KlineDatabase::KlineDatabase()
: db_(db_name, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE)
, table_(db_)
, stmtSelectDataByExchangeSymbolIndex(db_, "SELECT * FROM ? WHERE Exchange = ? and Symbol = ? and Index = ?;")
, stmtReplaceDataByExchangeSymbolIndex(db_, "REPLACE INTO ? VALUES (?,?,?,?,?,?,?);")
{

}

KlineDatabase::~KlineDatabase()
{
    if (loop_) {
        if (loop_->joinable()) {
            loop_->join();
        }
        delete loop_;
    }
}

void KlineDatabase::start()
{
    loop_ = new std::thread(&KlineDatabase::_loop, this);

}

void KlineDatabase::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init)
{
    _log_and_print("Database %s-%s add %lu klines", exchange.c_str(), symbol.c_str(), klines.size());

    std::unique_lock<std::mutex> inner_lock{ mutex_caches_ };
    switch( resolution )
    {
        case 60:
        {
            vector<KlineData>& data = caches_min1_[exchange][symbol];
            for( const auto& v : klines ) {
                if( data.size() == 0 || v.index > data.back().index ) {
                    data.push_back(v);
                } else if( v.index == data.back().index ) {
                    data.back() = v;
                } else {
                    _log_and_print("%s-%s kline min1 go back error", exchange.c_str(), symbol.c_str());
                }
            }
            break;
        }
        case 3600:
        {
            vector<KlineData>& data = caches_min60_[exchange][symbol];
            for( const auto& v : klines ) {
                if( data.size() == 0 || v.index > data.back().index ) {
                    data.push_back(v);
                } else if( v.index == data.back().index ) {
                    data.back() = v;
                } else {
                    _log_and_print("%s-%s kline min60 go back error", exchange.c_str(), symbol.c_str());
                }
            }
            break;
        }
        default:
        {
            _log_and_print("%s-%s unknown resolution %d", exchange.c_str(), symbol.c_str(), resolution);
            break;
        }
    }
}

bool KlineDatabase::get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    return true;
}

void KlineDatabase::_loop()
{
    while( true ) 
    {        
        unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min1;
        unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min60;
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_caches_ };
            caches_min1 = caches_min1_;
            caches_min1_.clear();
            caches_min60 = caches_min60_;
            caches_min60_.clear();
        }

        // 写入db
        _write_to_db(caches_min1, 60);
        _write_to_db(caches_min60, 3600);

        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

int timet_to_index(type_tick t, int resolution)
{
    struct tm *newtime = localtime( (const long int*)&t );
    if( resolution == 60 )
        return newtime->tm_year * 10000 + newtime->tm_mon * 100 + newtime->tm_mday;
    else if( resolution == 3600 )
        return newtime->tm_year * 100 + newtime->tm_mon;
    else
        return 0;
}
void KlineDatabase::_write_to_db_single(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines)
{
    int last_index = 0;
    vector<KlineData> tmp;
    for( size_t i = 0 ; i < klines.size() ; i ++ ) {
        int index = timet_to_index(klines[i].index, resolution);
        if( index == 0 )
            continue;
        if( index != last_index ) {
            last_index = index;
            tmp.clear();
            // 更新
            vector<KlineData> last_klines;
            _read_klines(exchange, symbol, resolution, last_index, last_klines);
            _mix_klines(last_klines, tmp);
            _write_klines(exchange, symbol, resolution, last_index, last_klines);
        } else {
            tmp.push_back(klines[i]);
        }
    }
    // 收尾
    if( tmp.size() > 0 ) {
        vector<KlineData> last_klines;
        _read_klines(exchange, symbol, resolution, last_index, last_klines);
        _mix_klines(last_klines, tmp);
        _write_klines(exchange, symbol, resolution, last_index, last_klines);
    }
}

void KlineDatabase::_write_to_db(const unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache, int resolution)
{
    switch( resolution) {
        case 60:
        {
            for( const auto& v: cache ) {
                for( const auto& v2 : v.second ) {
                    _write_to_db_single(v.first, v2.first, 60, v2.second);
                }
            }
            break;
        }
        case 3600:
        {
            for( const auto& v: cache ) {
                for( const auto& v2 : v.second ) {
                    _write_to_db_single(v.first, v2.first, 3600, v2.second);
                }
            }
            break;
        }
        default:
        {
            _log_and_print("unknown resolution %d", resolution);
            return;
        }
    }
}

bool KlineDatabase::_init_db()
{
    return true;
}

bool KlineDatabase::_write_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, const vector<KlineData>& klines)
{
    string table_name;
    switch( resolution ) {
        case 60:{
            table_name = KLINE_MIN1_TABLENAME;
            break;
        }
        case 3600:{
            table_name = KLINE_MIN60_TABLENAME;
            break;
        }
        default:{
            _log_and_print("%s-%s unknown resolution %d", exchange.c_str(), symbol.c_str(), resolution);
            return false;
        }
    }

    string data = encode_klines(klines);
    try
    {
        stmtReplaceDataByExchangeSymbolIndex.reset();
        SQLite::bind(stmtReplaceDataByExchangeSymbolIndex,
            table_name,
            exchange,
            symbol,
            index,
            data,
            0
        );
        stmtReplaceDataByExchangeSymbolIndex.exec();
        return true;
    }
    catch(const std::exception& e)
    {
        _log_and_print("[sqlite] [stmtReplaceDataByExchangeSymbolIndex] exception: %s", e.what());
        return false;
    }
    return true;
}

bool KlineDatabase::_read_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, vector<KlineData>& klines)
{
    string table_name;
    switch( resolution ) {
        case 60:{
            table_name = KLINE_MIN1_TABLENAME;
            break;
        }
        case 3600:{
            table_name = KLINE_MIN60_TABLENAME;
            break;
        }
        default:{
            _log_and_print("%s-%s unknown resolution %d", exchange.c_str(), symbol.c_str(), resolution);
            return false;
        }
    }
    
    try
    {
        stmtSelectDataByExchangeSymbolIndex.reset();
        SQLite::bind(stmtSelectDataByExchangeSymbolIndex,
            table_name,
            exchange,
            symbol,
            index
        );
        if (!stmtSelectDataByExchangeSymbolIndex.executeStep()) 
        {
            _log_and_print("[sqlite] [stmtSelectDataByExchangeSymbolIndex] executeStep fail");
            return false;
        }
        string data = stmtSelectDataByExchangeSymbolIndex.getColumn("Data").getString();
        return decode_json_klines(data, klines);
    }
    catch(const std::exception& e)
    {
        _log_and_print("[sqlite] [stmtSelectDataByExchangeSymbolIndex] exception: %s", e.what());
        return false;
    }
}
