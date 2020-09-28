#include "pandora/messager/ut_log.h"
#include "pandora/util/json.hpp"
#include "pandora/redis/redis_api.h"
#include "pandora/util/io_service_pool.h"

#include <unordered_map>
using namespace std;

class MarketDispatcher : public utrade::pandora::CRedisSpi
{
public:
    using RedisApiPtr = boost::shared_ptr<utrade::pandora::CRedisApi>;
    using UTLogPtr = boost::shared_ptr<utrade::pandora::UTLog>;
public:
    MarketDispatcher(const string& host, const int& port, const string& password) {
        UTLogPtr logger = utrade::pandora::UTLog::getStrategyLogger("market_dispatcher", "market_dispatcher");

        redis_syn_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
        redis_syn_api_->RegisterRedis(host, port, password, utrade::pandora::RM_GetData);


        redis_api_ = RedisApiPtr{new utrade::pandora::CRedisApi{logger}};
        // register callback
        redis_api_->RegisterSpi(this);
        // redis connector
        redis_api_->RegisterRedis(host, port, password, utrade::pandora::RM_Subscribe);
    }
    ~MarketDispatcher(){};

    // lauch the market data
    void Launch() {}

    void SubscribeByExchange(const string& exchangeName) {
        redis_api_->PSubscribeTopic("UPDATEx|*." + exchangeName);
    }

    void Subscribe(const string& symbol, const string& exchange) {
        //string depth_data = redis_api_->SyncGet("DEPTHx|" + symbol + "." + exchange);
        string depth_data = redis_syn_api_->SyncGet("DEPTHx|EOS_USDT.XDAEX");
        cout << "depth_data: " << depth_data << endl;
        redis_api_->SubscribeTopic("UPDATEx|" + symbol + "." + exchange);
    }

    void SubscribeAll() {
        redis_api_->PSubscribeTopic("*");
    }


    // redis connect notify
    virtual void OnConnected(){
        cout << "\n##### Redis MarketDispatcher::OnConnected ####\n" << endl;
    }
    // redis disconnect notify
    virtual void OnDisconnected(int status){
        cout << "\n##### Redis MarketDispatcher::OnDisconnected ####\n" << endl;
    }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg){
        cout << "\nReceive Msg: " << channel << " Msg: " << msg << "\n" << endl;
        // all channel
        if( all_channels_.find(channel) == all_channels_.end() ) {
            all_channels_[channel] = 1;
            cout << "find new channel:" << channel << endl;
        } else {
            all_channels_[channel] += 1;
        }
    }

private:
    RedisApiPtr redis_api_;
    RedisApiPtr redis_syn_api_;
    unordered_map<string, int>  all_channels_;
};

int main(int argc, char** argv) {
    string exchangeToSubscribe = "XDAEX";
    if( argc >= 2 ) {
        exchangeToSubscribe = argv[1];
    }
    cout << "subscribe exchange:" << exchangeToSubscribe << endl;

    using MarketDispatcherPtr = boost::shared_ptr<MarketDispatcher>;
    MarketDispatcherPtr market_dispatcher_ = boost::make_shared<MarketDispatcher>("45.249.244.59", 6666, "rkqFB4,wpoMmHqT6he}r");
    market_dispatcher_->Subscribe("EOS_USDT", "XDAEX");
    //market_dispatcher_->SubscribeByExchange(exchangeToSubscribe);
    //market_dispatcher_->SubscribeAll();
    market_dispatcher_->Launch();

    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}
