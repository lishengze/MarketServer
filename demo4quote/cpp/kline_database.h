#pragma once

// for leveldb
// #include "leveldb/db.h"
// for sqlite
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>


#include "stream_engine_define.h"

class IDataProvider
{
public:
    // 外部请求K线
    // resolution: 60 - min1 / 3600 - min60
    // start_time / end_time: 必须为秒数，不接受其他输入
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines) = 0;
    // 收到K线更新
    // resolution: 60 - min1 / 3600 - min60
    // is_init：是否第一次更新
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init) = 0;
};

#define KLINE_MIN1_TABLENAME     "kline_min1"
#define KLINE_MIN60_TABLENAME    "kline_min60"


class KlineDatabase : public IDataProvider
{
public:
    KlineDatabase();
    ~KlineDatabase();

    void start();

    // IDataProvider
    bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines);
    void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init);
private:
    mutable std::mutex mutex_caches_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min1_;
    unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>> caches_min60_;
    std::thread* _flush_thrd = nullptr;
    std::atomic<bool> thread_run_;
    void _flush_thread();

    void _write_to_db(const unordered_map<TExchange, unordered_map<TSymbol, vector<KlineData>>>& cache, int resolution);

    // db - sqlite
    // [table]kline_min1: 按日打包数据
    // [table]kline_min60：按月打包数据
    class Table {
    public:
        Table(SQLite::Database& db);
        ~Table() {}
    private:
        void createTableKlineMin1(SQLite::Database& db);
        void createTableKlineMin60(SQLite::Database& db);
    };
    SQLite::Database db_;
    Table table_;
    SQLite::Statement stmtMin1SelectDataByExchangeSymbolIndex;
    SQLite::Statement stmtMin1ReplaceDataByExchangeSymbolIndex;
    SQLite::Statement stmtMin1SelectDataByExchangeSymbolIndexRange;
    SQLite::Statement stmtMin60SelectDataByExchangeSymbolIndex;
    SQLite::Statement stmtMin60ReplaceDataByExchangeSymbolIndex;
    SQLite::Statement stmtMin60SelectDataByExchangeSymbolIndexRange;
    bool _write_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, const vector<KlineData>& klines);
    bool _read_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index, vector<KlineData>& klines);
    bool _read_range_klines(const TExchange& exchange, const TSymbol& symbol, int resolution, int index_begin, int index_end, vector<KlineData>& klines);
};
