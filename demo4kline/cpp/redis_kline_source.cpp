#include "redis_kline_source.h"
#include "kline_server_config.h"

RedisKlineSource::RedisKlineSource()
{

}

RedisKlineSource::~RedisKlineSource()
{

}

void RedisKlineSource::start()
{
    // 请求增量
    redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{CONFIG->logger_}};
    redis_api_->RegisterSpi(this);
    redis_api_->RegisterRedis(CONFIG->quote_redis_host_, CONFIG->quote_redis_port_, CONFIG->quote_redis_password_, utrade::pandora::RM_Subscribe);
}

void RedisKlineSource::OnConnected()
{
    _log_and_print("RedisKlineSource::OnConnected");
    for( auto iterSymbol = CONFIG->include_symbols_.begin() ; iterSymbol != CONFIG->include_symbols_.end() ; ++iterSymbol ) {
        for( auto iterExchange = CONFIG->include_exchanges_.begin() ; iterExchange != CONFIG->include_exchanges_.end() ; ++iterExchange ) {
            const string& symbol = *iterSymbol;
            const string& exchange = *iterExchange;
            string topic = "KLINEx|" + symbol + "." + exchange;
            _log_and_print("subscribe %s", topic.c_str());        
            redis_api_->SubscribeTopic(topic);
        }
    }
}

void RedisKlineSource::OnDisconnected(int status)
{
    _log_and_print("RedisKlineSource::OnDisconnected");
}

void RedisKlineSource::OnMessage(const std::string& channel, const std::string& msg)
{
    //cout << channel << ":" << msg << endl;    
    // 设置最近数据时间
    last_time_ = get_miliseconds();
    //cout << channel << endl;
    
    //std::cout << "update json size:" << msg.length() << std::endl;
    if (channel.find(MIN1_KLINE_PREFIX) != string::npos)
    {
        // decode exchange and symbol
        int pos = channel.find(MIN1_KLINE_PREFIX);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(MIN1_KLINE_PREFIX), pos2-pos-strlen(MIN1_KLINE_PREFIX));
        string exchange = channel.substr(pos2+1);

        njson snap_json = njson::parse(msg); 

        for (auto iter = snap_json.rbegin(); iter != snap_json.rend(); ++iter) {
            // 取最后一根
            const njson& data = *iter;
            KlineData kline;
            kline.index = int(data[0].get<float>());
            kline.px_open.from(data[1].get<double>());
            kline.px_high.from(data[2].get<double>());
            kline.px_low.from(data[3].get<double>());
            kline.px_close.from(data[4].get<double>());
            kline.volume = data[5].get<double>();
            _log_and_print("get kline %s index=%lu open=%s high=%s low=%s close=%s", channel.c_str(), 
                kline.index,
                kline.px_open.get_str_value().c_str(),
                kline.px_high.get_str_value().c_str(),
                kline.px_low.get_str_value().c_str(),
                kline.px_close.get_str_value().c_str()
                );
            for( auto& v : callbacks_ ) {
                vector<KlineData> datas;
                datas.push_back(kline);
                v->on_kline(exchange, symbol, 60, datas);
            }
            break;
        }
    }
}
