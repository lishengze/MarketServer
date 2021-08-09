#include "stream_engine_config.h"
#include "kline_database.h"

string db_name = "klines.dat";

// time_t转为时间索引
// resolution=60：YYYYMMDD
// resolution=3600: YYYYMM
int timet_to_index(type_tick t, int resolution)
{
    if( t == 0 )
        return 0;

    struct tm *newtime = localtime( (const long int*)&t );
    if( resolution == 60 ) {
        //return t;
        return (1900 + newtime->tm_year) * 10000 + (1 + newtime->tm_mon) * 100 + newtime->tm_mday;
    } else if( resolution == 3600 ) {
        return (1900 + newtime->tm_year) * 100 + (1 + newtime->tm_mon);
    } else {
        return 0;
    }
}

bool dbdata_to_kline(const njson& data, KlineData& kline) {
    kline.index = int(data[0].get<double>());
    kline.px_open.from(data[1].get<string>());
    kline.px_high.from(data[2].get<string>());
    kline.px_low.from(data[3].get<string>());
    kline.px_close.from(data[4].get<string>());
    kline.volume.from(data[5].get<string>());
    return true;
}

njson kline_to_dbdata( const KlineData& kline) {
    njson ret;
    ret.push_back(kline.index);
    ret.push_back(kline.px_open.get_str_value());
    ret.push_back(kline.px_high.get_str_value());
    ret.push_back(kline.px_low.get_str_value());
    ret.push_back(kline.px_close.get_str_value());
    ret.push_back(kline.volume.get_str_value());
    return ret;
}

string encode_dbdata(const vector<KlineData>& klines)
{
    njson data;
    for( const auto& v : klines ) {
        data.push_back(kline_to_dbdata(v));
    }

    return data.dump();
}

bool decode_dbdata( const string& data, vector<KlineData>& klines)
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
        dbdata_to_kline(*iter, kline);
        klines.push_back(kline);
    }
    return true;
}

KlineDatabase::Table::Table(SQLite::Database& db) 
{
    createTableKlineMin1(db);
    createTableKlineMin60(db);
    cout << "table init finish." << endl;
}

void KlineDatabase::Table::createTableKlineMin1(SQLite::Database& db) {
    try
    {
        if (db.tableExists(KLINE_MIN1_TABLENAME)) {
            cout << KLINE_MIN1_TABLENAME << " exist" << endl;
            return; 
        }

        db.exec("CREATE TABLE " KLINE_MIN1_TABLENAME "(\
                Exchange TEXT, \
                Symbol TEXT, \
                TimeIndex INTEGER, \
                Data TEXT, \
                Ratio INTEGER, \
                CreateTime TEXT, \
                ModifyTime TEXT \
            );");

        db.exec("CREATE unique INDEX " KLINE_MIN1_TABLENAME "_kline_index ON " KLINE_MIN1_TABLENAME "(Exchange, Symbol, TimeIndex);");
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
            cout << KLINE_MIN60_TABLENAME << " exist" << endl;
            return; 
        }

        db.exec("CREATE TABLE " KLINE_MIN60_TABLENAME "(\
                Exchange TEXT, \
                Symbol TEXT, \
                TimeIndex INTEGER, \
                Data TEXT, \
                Ratio INTEGER, \
                CreateTime TEXT, \
                ModifyTime TEXT \
            );");

        db.exec("CREATE unique INDEX " KLINE_MIN60_TABLENAME "_kline_index ON " KLINE_MIN60_TABLENAME "(Exchange, Symbol, TimeIndex);");
    }
    catch(const std::exception& e)
    {
        _log_and_print("[KlineDatabase] SQLite exception: %s", e.what());
    }
}

KlineDatabase::KlineDatabase()
: thread_run_(true)
, db_(db_name, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE)
, table_(db_)
, stmtMin1SelectAllData(db_,  "SELECT * FROM " KLINE_MIN1_TABLENAME ";")
, stmtMin60SelectAllData(db_, "SELECT * FROM " KLINE_MIN60_TABLENAME ";")
, stmtMin1SelectDataByExchangeSymbolIndex(db_, "SELECT * FROM " KLINE_MIN1_TABLENAME " WHERE Exchange = ? and Symbol = ? and TimeIndex = ?;")
, stmtMin1ReplaceDataByExchangeSymbolIndex(db_, "REPLACE INTO " KLINE_MIN1_TABLENAME " VALUES (?,?,?,?,?,?,?);")
, stmtMin1SelectDataByExchangeSymbolIndexRange(db_, "SELECT * FROM " KLINE_MIN1_TABLENAME " WHERE Exchange = ? and Symbol = ? and (TimeIndex between ? and ?) order by TimeIndex DESC limit 30;")
, stmtMin60SelectDataByExchangeSymbolIndex(db_, "SELECT * FROM " KLINE_MIN60_TABLENAME " WHERE Exchange = ? and Symbol = ? and TimeIndex = ?;")
, stmtMin60ReplaceDataByExchangeSymbolIndex(db_, "REPLACE INTO " KLINE_MIN60_TABLENAME " VALUES (?,?,?,?,?,?,?);")
, stmtMin60SelectDataByExchangeSymbolIndexRange(db_, "SELECT * FROM " KLINE_MIN60_TABLENAME " WHERE Exchange = ? and Symbol = ? and (TimeIndex between ? and ?) order by TimeIndex DESC limit 30;")
{
    vector<KlineData> stored_data;
    // get_all_data(stored_data);
}

KlineDatabase::~KlineDatabase()
{
    thread_run_ = false;
    if (_flush_thrd) {
        if (_flush_thrd->joinable()) {
            _flush_thrd->join();
        }
        delete _flush_thrd;
    }
}

void KlineDatabase::start()
{
    _flush_thrd = new std::thread(&KlineDatabase::_flush_thread, this);

}

void KlineDatabase::get_all_data(vector<KlineData>& result)
{
    try
    {
        get_db_data(stmtMin1SelectAllData, 60, result);

        get_db_data(stmtMin60SelectAllData, 3600, result);
    }
    catch(const std::exception& e)
    {
        std::cerr <<"\n[E]KlineDatabase::get_all_data " << e.what() << '\n';
    }    
}

void KlineDatabase::get_data(unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& result, int resolution)
{
    try
    {
        

        if (resolution == 3600)
        {
            get_db_data(stmtMin60SelectAllData, resolution, result);
        }

        if (resolution == 60)
        {
            get_db_data(stmtMin1SelectAllData, resolution, result);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "\n[E] " << e.what() << '\n';
    }
    
}

void KlineDatabase::get_db_data(SQLite::Statement& stmt, int resolution, unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& result)
{
    try
    {
        while(stmt.executeStep())
        {
            vector<KlineData> kline_data_vec;
            
            string data = stmt.getColumn("Data").getString();

            decode_dbdata(data, kline_data_vec);

            string exchange = stmt.getColumn("Exchange").getString();
            string symbol = stmt.getColumn("Symbol").getString();         

            // cout << "exchange: " << exchange << ", symbol: " << symbol << ", resolution: " << resolution << ", data_count: " << kline_data_vec.size() << endl;
            
            for (auto& kline_data:kline_data_vec)
            {
                kline_data.exchange = exchange;
                kline_data.symbol = symbol;
                kline_data.resolution = resolution;

                result[exchange][symbol].emplace_back(kline_data);
            }

            kline_data_vec.clear();            
        }           
    }
    catch(const std::exception& e)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << " " <<  e.what() << '\n';
    }

}

void KlineDatabase::get_db_data(SQLite::Statement& stmt, int resolution, vector<KlineData>& result)
{
    try
    {
        cout << "get_db_data " << resolution << endl;

        while(stmt.executeStep())
        {
            vector<KlineData> kline_data_vec;
            
            string data = stmt.getColumn("Data").getString();

            decode_dbdata(data, kline_data_vec);

            string exchange = stmt.getColumn("Exchange").getString();
            string symbol = stmt.getColumn("Symbol").getString();         

            cout << "exchange: " << exchange << ", symbol: " << symbol << ", resolution: " << resolution << ", data_count: " << kline_data_vec.size() << endl;
              
            for (auto& kline_data:kline_data_vec)
            {
                kline_data.exchange = exchange;
                kline_data.symbol = symbol;
                kline_data.resolution = resolution;

                result.emplace_back(kline_data);
            }

            kline_data_vec.clear();            
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"\n[E] KlineDatabase::get_db_data " << e.what() << '\n';
    }

}

void KlineDatabase::on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init)
{
    //_log_and_print("Database %s.%s kline%u add %lu klines", exchange, symbol, resolution, klines.size());

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
                    _log_and_print("%s.%s kline min1 go back error", exchange, symbol);
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
                    _log_and_print("%s.%s kline min60 go back error", exchange, symbol);
                }
            }
            break;
        }
        default:
        {
            _log_and_print("%s.%s unknown resolution %d", exchange, symbol, resolution);
            break;
        }
    }
}

bool KlineDatabase::get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
{
    klines.clear();
    int begin_index = timet_to_index(start_time, resolution);
    int end_index = timet_to_index(end_time, resolution);
    // end_index无效的时候设置为最大值
    if( end_index == 0 )
        end_index = 999999999;
    return _read_range_klines(exchange, symbol, resolution, begin_index, end_index, klines);
}

void KlineDatabase::_flush_thread()
{
    while( thread_run_ ) 
    {
        unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min1;
        unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min60;
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_caches_ };
            caches_min1.swap(caches_min1_);
            caches_min60.swap(caches_min60_);
        }

        // 写入db
        _write_to_db(caches_min1, 60);
        _write_to_db(caches_min60, 3600);

        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

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

void KlineDatabase::_write_to_db(const unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache, int resolution)
{
    for( const auto& v: cache ) {
        for( const auto& v2: v.second ) {
            const TExchange& exchange = v.first;
            const TSymbol& symbol = v2.first;
            const vector<KlineData>& klines = v2.second;
            
            int cur_index = 0;
            int last_index = 0;
            vector<KlineData> tmp;

            for( size_t i = 0 ; i < klines.size() ; i ++ ) {

                // 计算索引值
                cur_index = timet_to_index(klines[i].index, resolution);
                if( cur_index == 0 )
                    continue;


                if( cur_index != last_index && last_index != 0) {
                    last_index = cur_index;
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
                _read_klines(exchange, symbol, resolution, cur_index, last_klines);
                _mix_klines(last_klines, tmp);
                _write_klines(exchange, symbol, resolution, cur_index, last_klines);
            }
        }
    }
}

bool KlineDatabase::_write_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, const vector<KlineData>& klines)
{
    string data = encode_dbdata(klines);

    try
    {
        switch( resolution )
        {
            case 60:
            {
                stmtMin1ReplaceDataByExchangeSymbolIndex.reset();
                SQLite::bind(stmtMin1ReplaceDataByExchangeSymbolIndex,
                    exchange,
                    symbol,
                    index,
                    data,
                    0,
                    "",                    //CreateTime
                    ""                    //ModifyTime
                );
                stmtMin1ReplaceDataByExchangeSymbolIndex.exec();
                break;
            }
            case 3600:
            {
                stmtMin60ReplaceDataByExchangeSymbolIndex.reset();
                SQLite::bind(stmtMin60ReplaceDataByExchangeSymbolIndex,
                    exchange,
                    symbol,
                    index,
                    data,
                    0,
                    "",                    //CreateTime
                    ""                    //ModifyTime
                );
                stmtMin60ReplaceDataByExchangeSymbolIndex.exec();
                break;
            }
            default:
            {
                _log_and_print("%s.%s unknown resolution %d", exchange, symbol, resolution);
                return false;
            }
        }
        return true;
    }
    catch(const std::exception& e)
    {
        _log_and_print("[sqlite] [stmtReplaceDataByExchangeSymbolIndex] exception: %s", e.what());
        return false;
    }
    return true;
}

bool KlineDatabase::_read_range_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index_begin, int index_end, vector<KlineData>& klines)
{    
    _log_and_print("%s.%s symbol resolution=%d begin=%d end=%d", exchange, symbol, resolution, index_begin, index_end);
    klines.clear();

    string data;
    vector<KlineData> _klines;
    try
    {
        switch( resolution ) 
        {
            case 60:
            {
                stmtMin1SelectDataByExchangeSymbolIndexRange.reset();
                SQLite::bind(stmtMin1SelectDataByExchangeSymbolIndexRange,
                    exchange,
                    symbol,
                    index_begin,
                    index_end
                );
                while(stmtMin1SelectDataByExchangeSymbolIndexRange.executeStep()) {
                    int idx = stmtMin1SelectDataByExchangeSymbolIndexRange.getColumn("TimeIndex").getInt();
                    data = stmtMin1SelectDataByExchangeSymbolIndexRange.getColumn("Data").getString();
                    decode_dbdata(data, _klines);
                    klines.insert(klines.begin(), _klines.begin(), _klines.end());
                    _log_and_print("get index=%d size=%lu", idx, _klines.size());
                }
                break;
            }
            case 3600:
            {
                stmtMin60SelectDataByExchangeSymbolIndexRange.reset();
                SQLite::bind(stmtMin60SelectDataByExchangeSymbolIndexRange,
                    exchange,
                    symbol,
                    index_begin,
                    index_end
                );
                while(stmtMin60SelectDataByExchangeSymbolIndexRange.executeStep()) {
                    data = stmtMin60SelectDataByExchangeSymbolIndexRange.getColumn("Data").getString();
                    decode_dbdata(data, _klines);
                    klines.insert(klines.begin(), _klines.begin(), _klines.end());
                }
                break;
            }
            default:{
                _log_and_print("%s.%s unknown resolution %d", exchange, symbol, resolution);
                return false;
            }
        }
    }
    catch(const std::exception& e)
    {
        _log_and_print("[sqlite] [stmtSelectDataByExchangeSymbolIndex] exception: %s", e.what());
        return false;
    }
    
    return true;
}

bool KlineDatabase::_read_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, vector<KlineData>& klines)
{
    string data;
    try
    {
        switch( resolution ) 
        {
            case 60:
            {
                stmtMin1SelectDataByExchangeSymbolIndex.reset();
                SQLite::bind(stmtMin1SelectDataByExchangeSymbolIndex,
                    exchange,
                    symbol,
                    index
                );
                if (!stmtMin1SelectDataByExchangeSymbolIndex.executeStep()) 
                {
                    _log_and_print("[sqlite] [stmtMin1SelectDataByExchangeSymbolIndex] executeStep fail");
                    return false;
                }
                data = stmtMin1SelectDataByExchangeSymbolIndex.getColumn("Data").getString();
                break;
            }
            case 3600:
            {
                stmtMin60SelectDataByExchangeSymbolIndex.reset();
                SQLite::bind(stmtMin60SelectDataByExchangeSymbolIndex,
                    exchange,
                    symbol,
                    index
                );
                if (!stmtMin60SelectDataByExchangeSymbolIndex.executeStep()) 
                {
                    _log_and_print("[sqlite] [stmtMin60SelectDataByExchangeSymbolIndex] executeStep fail");
                    return false;
                }
                data = stmtMin60SelectDataByExchangeSymbolIndex.getColumn("Data").getString();
                break;
            }
            default:{
                _log_and_print("%s.%s unknown resolution %d", exchange, symbol, resolution);
                return false;
            }
        }
    }
    catch(const std::exception& e)
    {
        _log_and_print("[sqlite] [stmtSelectDataByExchangeSymbolIndex] exception: %s", e.what());
        return false;
    }
    
    return decode_dbdata(data, klines);
}
