#pragma once

#include "stream_engine_define.h"

string make_fake_symbol(const string& channel_type, int id, const TSymbol& symbol, const TExchange& exchange);

class QuoteDumper
{
public:
    void start();

    void add_message(const string& channel, const string& msg);

private:
    std::thread* checker_loop_ = nullptr;
    std::atomic<bool> thread_run_;

    void _dump_thread();

    moodycamel::ConcurrentQueue<string> pkgs_;
};


class RedisQuote;
class QuoteReplayer
{
public:
    void init(RedisQuote* ptr, int ratio = 2, int replicas = 0, const string& filepath = "dump.dat");

    bool start();
private:
    // callback
    RedisQuote *quote_interface_ = nullptr;
    // 文件名
    string filepath_;
    // 回放速度
    int ratio_;
    // btc行情复制份数
    int replicas_;

    std::thread* thread_ = nullptr;

    void _load_and_send();

    bool _get_pkg(ifstream& fin, type_tick& ts, string& channel, string& msg);

    void _send_pkg(const string& channel, const string& msg);
};
