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
    uint32 exchange_length;
    double amount_cost; // 余额消耗量

    SInnerDepth() {
        total_volume = 0;
        exchange_length = 0;
    }

    void mix_exchanges(const SInnerDepth& src, double bias) {
        for( uint32 i = 0 ; i < src.exchange_length ; ++i ) {
            double biasedVolume = src.exchanges[i].volume * ( 100 + bias ) / 100.0;
            bool found = false;
            for( unsigned int j = 0 ; j < exchange_length ; ++j ) {
                if( string(exchanges[j].name) == string(src.exchanges[i].name) ) {
                    exchanges[j].volume += biasedVolume;
                    found = true;
                    break;
                }
            }
            if( !found ) {
                vassign(exchanges[exchange_length].name, MAX_EXCHANGENAME_LENGTH, src.exchanges[i].name);
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
    uint32 ask_length;
    SInnerDepth bids[MAX_DEPTH_LENGTH];
    uint32 bid_length;

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



struct Params {
    AccountInfo cache_account;
    QuoteConfiguration cache_config;
    unordered_map<TSymbol, pair<vector<SOrderPriceLevel>, vector<SOrderPriceLevel>>> cache_order;
};


// 流水线模型
struct PipelineContent
{
    Params params;
    bool is_sample_;
};

class Worker
{
public:
    Worker() {}
    virtual ~Worker() {}

    void add_worker(Worker* w) {
        this->next_ = w;
    }

    SInnerQuote* run(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out) {
        SInnerQuote* tmp = this->process(src, ctx, out);
        if( next_ ) {
            return next_->run(tmp, ctx, out);
        } else {
            return tmp;
        }
    }

    virtual SInnerQuote* process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out) = 0;
private:
    Worker *next_ = nullptr;
};

class QuotePipeline
{
public:
    QuotePipeline(){}
    virtual ~QuotePipeline(){}

    void add_worker(Worker* w) {
        if( head_ == nullptr ) {
            head_ = w;
        }
        if( tail_ == nullptr ) {
            tail_ = w;
        } else {
            tail_->add_worker(w);
            tail_ = w;
        }
    }

    void run(const SInnerQuote& quote, const Params& params, SInnerQuote& newQuote) {
        PipelineContent ctx;
        ctx.params = params;
        if( quote.symbol == CONFIG->sample_symbol_ ) {
            ctx.is_sample_ = true;
        } else {
            ctx.is_sample_ = false;
        }

        if( !head_ )
            return;

        SInnerQuote tmp = quote;
        SInnerQuote* ret = head_->run(&tmp, ctx, &newQuote);
        newQuote = *ret;
    }
private:
    Worker *head_ = nullptr, *tail_ = nullptr;
};

// worker： 处理买卖一价位交叉问题
struct SymbolWatermark
{
    SDecimal watermark;
    unordered_map<TExchange, SDecimal> asks;
    unordered_map<TExchange, SDecimal> bids;
};
class WatermarkComputerWorker : public Worker
{
public:
    WatermarkComputerWorker();
    ~WatermarkComputerWorker();

    bool get_watermark(const string& symbol, SDecimal& watermark) const;
    void set_snap(const SInnerQuote& quote);

private:
    // 独立线程计算watermark
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, SymbolWatermark*> watermark_;
    std::thread* thread_loop_ = nullptr;
    void _calc_watermark();

    virtual SInnerQuote* process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out);
};

// worker：处理对冲账户资金量风控
class AccountAjdustWorker: public Worker
{
public:
    AccountAjdustWorker(){}
    ~AccountAjdustWorker(){};
    void set_snap(const SInnerQuote& quote);
private:
    // cache
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, int> currency_count_;
    unordered_set<TSymbol> symbols_;
    virtual SInnerQuote* process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out);

    bool get_currency(const SInnerQuote& quote, string& sell_currency, int& sell_count, string& buy_currency, int& buy_count) const;
};

// worker：处理订单簿风控
class OrderBookWorker : public Worker
{
public:
    OrderBookWorker(){}
    ~OrderBookWorker(){}
private:
    virtual SInnerQuote* process(SInnerQuote* src, PipelineContent& ctx, SInnerQuote* out);
};

class DataCenter {
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
private:
    void _add_quote(const SInnerQuote& src, SInnerQuote& dst, Params& params);

    void _publish_quote(const SInnerQuote& quote, const Params& params);

    void _push_to_clients(const string& symbol = "");

    //void _calc_newquote(const SInnerQuote& quote, const Params& params, SInnerQuote& newQuote);
    
    mutable std::mutex                  mutex_datas_;
    unordered_map<TSymbol, SInnerQuote> datas_;

    Params params_;

    // 处理流水线
    QuotePipeline pipeline_;
    WatermarkComputerWorker watermark_worker_;
    AccountAjdustWorker account_worker_;
    OrderBookWorker orderbook_worker_;
};