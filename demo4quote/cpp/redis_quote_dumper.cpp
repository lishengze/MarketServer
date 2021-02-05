#include "redis_quote_dumper.h"
#include "redis_quote.h"

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
    
    if( replicas_ == 0 ) {
        quote_interface_->on_message(channel, msg, retry);
        return;
    }

    string channel_type;
    TSymbol symbol;
    TExchange exchange;
    if( !decode_channelname(channel, channel_type, symbol, exchange) )
        return;
        
    for( int i = 0 ; i < replicas_ ; i++ )
    {
        string _channel = make_fake_symbol(channel_type, i, symbol, exchange);
        quote_interface_->on_message(_channel, msg, retry);
    }
}

void QuoteReplayer::_load_and_send()
{
    if( ratio_ < 1 ) {
        std::cout << "unsurpport ratio " << ratio_  << std::endl;
        return;
    }

    ifstream fin(filepath_, ios::in);
    if( !fin ) {
        std::cout << "open " << filepath_ << " failed." << std::endl;
        return;
    }

    type_tick start_time = get_miliseconds(); // 以开始时间作为基准
    type_tick first_pkg_time = 0; // 第一个数据包的时间
    bool exit = false; // 中止发送
    string line; // 读取一行的内容

    type_tick ts = 0; // 解压后时间戳
    string channel, msg; // 解压后的频道和消息

    while( !exit ) 
    {
        type_tick now = get_miliseconds();
        type_tick limit_timespan = (now - start_time) * ratio_; // 时间范围在 first_pkg_time + limit_timespan 之间的允许发送

        if( first_pkg_time == 0 ) {
            if( !_get_pkg(fin, ts, channel, msg) ) {
                std::cout << "get_pkg fail1" << endl;
                exit = true;
                break;
            }
            first_pkg_time = ts;

            _send_pkg(channel, msg);
        }

        while( !exit ) {
            if( ts != 0 ) {
                _send_pkg(channel, msg);
            }

            if( !_get_pkg(fin, ts, channel, msg) ) {
                std::cout << "get_pkg fail2" << endl;
                exit = true;
                break;
            }

            //cout << ts << ", " << first_pkg_time << ", " << limit_timespan << endl;
            if( ts >= (first_pkg_time + limit_timespan ) ) {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    fin.close();
    std::cout << "exit replay." << std::endl;
    return;
}