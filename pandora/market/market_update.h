#pragma once
#include "../pandora_declare.h"
#include "../messager/messager.h"
#include "../util/thread_basepool.h"
#include "../messager/messager.h"
#include "../util/singleton.hpp"
#include "../messager/ut_log.h"
#include "../redis/redis_api.h"
#include "../util/json.hpp"
#include <mutex>

PANDORA_NAMESPACE_START


#define SUBSCRIBE_CURRENCY_VALUE(item) \
    if (market_) market_->subscribe_value_currency(item);

#define SUBSCRIBE_SYMBOL_MARKET(item) \
    if (market_) market_->subscribe_symbol_market(item);

// the interface for market_callbk callback
class IMarketCallback
{
public:
    virtual void on_value_currency(std::unordered_map<string, double>&) {}
    virtual void on_symbol_market(std::unordered_map<string, double>& ) {}
};
DECLARE_PTR(IMarketCallback);

// define the unique object
#define MARKET_AGENT Singleton<MarketAgent>::Instance()
#define DESTROY_MARKET_AGENT Singleton<MarketAgent>::DestroyInstance()

// market agent 
class MarketAgent : ThreadBasePool
{
public:
    MarketAgent(io_service_pool& pool, UTLogPtr logger) : ThreadBasePool{pool}, timer_{new boost::asio::deadline_timer{pool.get_io_service()}}, logger_{logger}, io_srvc{nullptr} 
    {
    }
    virtual ~MarketAgent() {}

    void launch(boost::asio::io_service& io_service)
    {
        io_srvc = &io_service;
        UT_LOG_INFO(logger_, "[MarketAgent] [launch]");
        get_io_service().post(std::bind(&MarketAgent::on_timer, this, 1000));
    }

    // subscribe the value of all kind of currency
    void subscribe_value_currency(IMarketCallback* market_callbk)
    {
        std::unique_lock<std::mutex> lock{mutex_visit_currency_};
        currency_market_.emplace(market_callbk);
    }

    // subscribe the symbol 
    void subscribe_symbol_market(IMarketCallback* market_callbk)
    {
        std::unique_lock<std::mutex> lock{mutex_visit_symbol_};
        symbol_market_.emplace(market_callbk);
    }

    // on timer event
    void on_timer(int millisec)
    {
        check_market_data();    // get the latest data of currency

        // timer invoke some microseconds later
        timer_->expires_from_now(boost::posix_time::milliseconds(millisec));
        // call the asys call function
        timer_->async_wait(boost::bind(&MarketAgent::on_timer, this, millisec));
    }

    // set server config 
    void set_server_config(const string& host, int port, const string& auth)
    {
        // generate redis api
        redis_api_ = CRedisApiPtr{new CRedisApi{logger_}};
        // redis connector
        redis_api_->RegisterRedis(host, port, auth, RM_GetData);
        // try to start receiver component
        UT_LOG_INFO(logger_, "[MarketAgent] Start Redis Host: " << host << " Port: " << port);
    }

    // check market data 
    void check_market_data()
    {
        // read the currency evaluation
        string data_str = redis_api_->SyncGet("LAST_MATCH");
        if (data_str.empty())   return;
        if (!data_str.empty() && io_srvc)
        {
            boost::shared_ptr<string> last_match_str{new string{data_str}};
            io_srvc->post(std::bind(&MarketAgent::handle_last_match, this, last_match_str));
        }
        // nlohmann::json js = nlohmann::json::parse(data_str);
        // for (nlohmann::json::const_iterator iter = js.begin(); iter != js.end(); ++iter)
        // {
        //     symbol_last_match[iter.key()] = double(iter.value());
        //     // std::cout << iter.key() << " : " << iter.value() << std::endl;
        // }
        // {
        //     std::unique_lock<std::mutex> lock{mutex_visit_currency_};
        //     for_each(symbol_market_.begin(), symbol_market_.end(), [&](std::set<IMarketCallback*>::value_type symbol_callbk)
        //     {
        //         symbol_callbk->on_symbol_market(symbol_last_match);
        //     });
        // }

        // read the last price of instrument
        data_str = redis_api_->SyncGet("FAIRVALUE");
        if (data_str.empty())   return;
        if (!data_str.empty() && io_srvc)
        {
            boost::shared_ptr<string> fair_value_str{new string{data_str}};
            io_srvc->post(std::bind(&MarketAgent::handle_fair_value, this, fair_value_str));
        }
        // js = nlohmann::json::parse(data_str);
        // // std::cout << js["TOKEN_PRICE"] << std::endl;
        // for (nlohmann::json::const_iterator iter = js["TOKEN_PRICE"].begin(); iter != js["TOKEN_PRICE"].end(); ++iter)
        // {
        //     symbol_evaluation[iter.key()] = double(iter.value()[0]);
        //     // std::cout << iter.key() << " : " << iter.value()[0] << std::endl;
        // }
        // {
        //     std::unique_lock<std::mutex> lock{mutex_visit_symbol_};
        //     for_each(currency_market_.begin(), currency_market_.end(), [&](std::set<IMarketCallback*>::value_type currency_callbk)
        //     {
        //         currency_callbk->on_value_currency(symbol_evaluation);
        //     });   
        // }
    }

private:
    void handle_last_match(boost::shared_ptr<string> last_match_str)
    {
        std::unordered_map<string, double> symbol_last_match;
        nlohmann::json js = nlohmann::json::parse(*last_match_str.get());
        for (nlohmann::json::const_iterator iter = js.begin(); iter != js.end(); ++iter)
        {
            symbol_last_match[iter.key()] = double(iter.value());
            // std::cout << iter.key() << " : " << iter.value() << std::endl;
        }
        {
            std::unique_lock<std::mutex> lock{mutex_visit_currency_};
            for_each(symbol_market_.begin(), symbol_market_.end(), [&](std::set<IMarketCallback*>::value_type symbol_callbk)
            {
                symbol_callbk->on_symbol_market(symbol_last_match);
            });
        }
    }

    void handle_fair_value(boost::shared_ptr<string> fair_value_str)
    {
        std::unordered_map<string, double> symbol_evaluation;
        nlohmann::json js = nlohmann::json::parse(*fair_value_str.get());
        // std::cout << js["TOKEN_PRICE"] << std::endl;
        for (nlohmann::json::const_iterator iter = js["TOKEN_PRICE"].begin(); iter != js["TOKEN_PRICE"].end(); ++iter)
        {
            symbol_evaluation[iter.key()] = double(iter.value()[0]);
            // std::cout << iter.key() << " : " << iter.value()[0] << std::endl;
        }
        {
            std::unique_lock<std::mutex> lock{mutex_visit_symbol_};
            for_each(currency_market_.begin(), currency_market_.end(), [&](std::set<IMarketCallback*>::value_type currency_callbk)
            {
                currency_callbk->on_value_currency(symbol_evaluation);
            });   
        }
    }
    // pickup market data accord to redis key
    bool pickup_market_data(std::unordered_map<string, double>& datas, const string& market_key, const string& item_key, const string& item_value)
    {
        string data_str = redis_api_->SyncGet(market_key);
        nlohmann::json js = nlohmann::json::parse(data_str);
        for (auto& data : js)
        {
            datas[data[item_key]] = double(data[item_value]);
        }
        return true;
    }
    // the currency callback objects
    std::set<IMarketCallback*> currency_market_;
    // the symbol callback object
    std::set<IMarketCallback*> symbol_market_;
    // timer object
    boost::shared_ptr<boost::asio::deadline_timer> timer_;
    // redis api, to send the request to server
    CRedisApiPtr redis_api_;
    // logger
    UTLogPtr logger_;
    // mutext to visit
	std::mutex mutex_visit_currency_;
    std::mutex mutex_visit_symbol_;
    // the ioserv object
    boost::asio::io_service* io_srvc;
};
DECLARE_PTR(MarketAgent);

PANDORA_NAMESPACE_END