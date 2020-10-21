#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <mutex>
using namespace std;
#include "updater_configuration.h"
#include "updater_account.h"
#include "updater_order.h"
#include "risk_controller_define.h"


// 内部行情结构
struct SExchangeData {
    char name[MAX_EXCHANGENAME_LENGTH];
    double volume;

    SExchangeData() {
        strcpy(name, "");
        volume = 0;
    }
};

struct SInnerDepth {
    SDecimal price;
    double total_volume; // 总挂单量，用于下发行情
    SExchangeData exchanges[MAX_EXCHANGE_LENGTH];
    int exchange_length;
    double amount_cost; // 余额消耗量

    SInnerDepth() {
        total_volume = 0;
        exchange_length = 0;
    }

    void mix_exchanges(const SInnerDepth& src, double bias) {
        for( int i = 0 ; i < src.exchange_length ; ++i ) {
            double biasedVolume = src.exchanges[i].volume * bias;
            bool found = false;
            for( int j = 0 ; j < exchange_length ; ++j ) {
                if( string(exchanges[j].name) == string(src.exchanges[i].name) ) {
                    exchanges[j].volume += biasedVolume;
                    found = true;
                    break;
                }
            }
            if( !found ) {
                strcpy(exchanges[exchange_length].name, src.exchanges[i].name);
                exchanges[exchange_length].volume = biasedVolume;
                exchange_length ++;
            }
        }
    }
};

struct SInnerQuote {
    char symbol[MAX_SYMBOLNAME_LENGTH];
    long long time;
    long long time_arrive;
    long long seq_no;
    SInnerDepth asks[MAX_DEPTH_LENGTH];
    int ask_length;
    SInnerDepth bids[MAX_DEPTH_LENGTH];
    int bid_length;

    SInnerQuote() {
        strcpy(symbol, "");
        time = 0;
        time_arrive = 0;
        seq_no = 0;
        ask_length = 0;
        bid_length = 0;
    }
};

class CallDataServeMarketStream;
class DataCenter {
public:
    struct Params {
        AccountInfo cache_account;
        QuoteConfiguration cache_config;
        unordered_map<TSymbol, pair<vector<SOrderPriceLevel>, vector<SOrderPriceLevel>>> cache_order;
    };
public:
    DataCenter();
    ~DataCenter();

    // 回调行情通知
    void add_quote(const SInnerQuote& quote);
    // 触发重新计算，并下发行情给所有client
    void change_account(const AccountInfo& info);
    // 触发重新计算，并下发行情给所有client
    void change_configuration(const QuoteConfiguration& config);
    // 触发指定品种重新计算，并下发该品种行情给所有client
    void change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids);

    // 推送客户端注册
    void add_client(CallDataServeMarketStream* client);
    void del_client(CallDataServeMarketStream* client);
private:
    void _publish_quote(const SInnerQuote& quote, const Params& params);

    void _update_clients(const string& symbol = "");

    void _calc_newquote(const SInnerQuote& quote, const Params& params, SInnerQuote& newQuote);
    
    mutable std::mutex                  mutex_datas_;
    unordered_map<TSymbol, SInnerQuote> datas_;
    
    mutable std::mutex                              mutex_clients_;
    unordered_map<CallDataServeMarketStream*, bool> clients_;

    Params params_;

    // cache
    unordered_map<string, int> currency_count_;
};