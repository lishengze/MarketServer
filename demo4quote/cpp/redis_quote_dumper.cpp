#include "redis_quote_dumper.h"
#include "redis_quote.h"
#include "stream_engine_config.h"
#include "redis_quote.h" // for decode_channelname
// json库
#include "base/cpp/rapidjson/document.h"
#include "base/cpp/rapidjson/writer.h"
#include "base/cpp/rapidjson/stringbuffer.h"
using namespace rapidjson;

string make_fake_symbol(const string& channel_type, int id, const TSymbol& symbol, const TExchange& exchange)
{
    return tfm::format("%s|%s_%d.%s", channel_type, symbol, id, exchange);
}

string encode_msg(const string& channel, const string& msg){
    return tfm::format("%u,%s,%s\n", get_miliseconds(), channel, msg);
}

bool decode_msg(const string& raw, type_tick& ts, string& channel, string& msg)
{
    std::string::size_type pos = raw.find(",");
    if( pos == std::string::npos )
        return false;
    std::string::size_type pos2 = raw.find(",", pos+1);
    if( pos2 == std::string::npos )
        return false;

    // ts
    ts = ToUint64(raw.substr(0, pos));

    // channel
    channel = raw.substr(pos+1, pos2-pos-1);

    // msg
    msg = raw.substr(pos2+1);

    /*
    cout << raw << endl;
    cout << pos << " " << pos2 << endl;
    cout << ts << endl;
    cout << channel << endl;
    cout << msg << endl;*/
    return true;
}

void QuoteDumper::start() {
    thread_run_ = true;
    checker_loop_ = new std::thread(&QuoteDumper::_dump_thread, this);
}

void QuoteDumper::add_message(const string& channel, const string& msg) 
{
    pkgs_.enqueue(encode_msg(channel, msg));
}

void QuoteDumper::_dump_thread()
{
    FILE *f = fopen("dump.dat", "w");

    string holder[1024];

    while( thread_run_ )
    {
        size_t count = pkgs_.try_dequeue_bulk(holder, 1024);
        for( size_t i = 0 ; i < count ; i ++ ) {
            fwrite(holder[i].c_str(), holder[i].length(), 1, f);
        }
        fflush(f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    fclose(f);
}

void QuoteReplayer::init(RedisQuote* ptr, int ratio, int replicas, const string& filepath) { 
    filepath_ = filepath;
    ratio_ = ratio;
    replicas_ = replicas;
    quote_interface_ = ptr; 
}

bool QuoteReplayer::start() {
    if( thread_ )
        return false;
    thread_ = new std::thread(&QuoteReplayer::_load_and_send, this);
    return true;
}

bool QuoteReplayer::_get_pkg(ifstream& fin, type_tick& ts, string& channel, string& msg)
{
    string line;
    if( !getline(fin, line)  )
        return false;
    //cout << line << endl;
    if( !decode_msg(line, ts, channel, msg) )
        return false;

    return true;
}

void QuoteReplayer::_send_pkg(const string& channel, const string& msg)
{
    bool retry;
    
    string channel_type;
    TSymbol symbol;
    TExchange exchange;
    if( !decode_channelname(channel, channel_type, symbol, exchange) )
        return;
        
    quote_interface_->on_message(channel, msg, retry);
    for( int i = 1 ; i <= replicas_ ; i++ )
    {
        string _channel = make_fake_symbol(channel_type, i, symbol, exchange);
        quote_interface_->on_message(_channel, msg, retry);
    }
}

struct record
{
    type_tick ts;
    string channel;
    string msg;
};

bool pop_record(queue<record>& records, record& r) {
    if( records.size() == 0 )
        return false;
    r = records.front();
    records.pop();
    return true;
}

void QuoteReplayer::_load_and_send()
{
    _log_and_print("QuoteReplayer starting: ratio=%d replicas=%d", ratio_, replicas_);

    if( ratio_ < 1 ) {
        _log_and_print("unsurpport ratio %d", ratio_);
        return;
    }

    ifstream fin(filepath_, ios::in);
    if( !fin ) {
        _log_and_print("open %s failed", filepath_);
        return;
    }

    type_tick start_time = get_miliseconds(); // 以开始时间作为基准
    type_tick first_pkg_time = 0; // 第一个数据包的时间
    bool exit = false; // 中止发送
    string line; // 读取一行的内容

    type_tick ts = 0; // 解压后时间戳
    string channel, msg; // 解压后的频道和消息

    // 预读取文件
    queue<record> records;
    vector<string> depths; // 用于统计纯json解析耗时
    vector<string> klines; // 用于统计纯json解析耗时
    vector<string> trades; // 用于统计纯json解析耗时
    while( true ) {
        if( !_get_pkg(fin, ts, channel, msg) ) {
            std::cout << "get_pkg fail1" << endl;
            break;
        }
        records.push(record{ts, channel, msg});
        
        string chl_type;
        TSymbol chl_symbol;
        TExchange chl_exchange;
        if( decode_channelname(channel, chl_type, chl_symbol, chl_exchange) ) {
            if( chl_type == "__SNAPx" || chl_type == "UPDATEx" ) {
                depths.push_back(msg);
            } else if( chl_type == "TRADEx" ) {
                trades.push_back(msg);
            } else {
                klines.push_back(msg);
            }

        }
    }

    // 统计解析耗时
    cout << "total " << depths.size() << " depths" << endl;
    {
        TimeCostWatcher w("parse depths", 0, 0);
        for( const auto& v : depths ){
            Document body;
            body.Parse(v.c_str());
            if(body.HasParseError())
            {
                cout << "parse depths error" << endl;
            }
        }
    }
    cout << "total " << klines.size() << " klines" << endl;
    {
        TimeCostWatcher w("parse klines", 0, 0);
        for( const auto& v : klines ){
            Document body;
            body.Parse(v.c_str());
            if(body.HasParseError())
            {
                cout << "parse klines error" << endl;
            }
        }
    }
    cout << "total " << trades.size() << " trades" << endl;
    {
        TimeCostWatcher w("parse trades", 0, 0);
        for( const auto& v : trades ){
            Document body;
            body.Parse(v.c_str());
            if(body.HasParseError())
            {
                cout << "parse trades error" << endl;
            }
        }
    }

    // 开始发送
    record r;
    while( !exit ) 
    {
        type_tick now = get_miliseconds();
        type_tick limit_timespan = (now - start_time) * ratio_; // 时间范围在 first_pkg_time + limit_timespan 之间的允许发送

        if( first_pkg_time == 0 ) {
            if( !pop_record(records, r) ) {
                exit = true;
                break;
            }
            first_pkg_time = r.ts;

            _send_pkg(r.channel, r.msg);
        }

        while( !exit ) {
            if( ts != 0 ) {
                _send_pkg(r.channel, r.msg);
            }
            
            if( !pop_record(records, r) ) {
                exit = true;
                break;
            }

            //cout << ts << ", " << first_pkg_time << ", " << limit_timespan << endl;
            if( r.ts >= (first_pkg_time + limit_timespan ) ) {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    fin.close();
    std::cout << "exit replay." << std::endl;
    return;
}