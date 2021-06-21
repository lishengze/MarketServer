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
#include "grpc_entity.h"


// 内部行情结构
struct SInnerDepth {
    SDecimal total_volume; // 总挂单量，用于下发行情
    map<TExchange, SDecimal> exchanges;
    //double amount_cost; // 余额消耗量

    SInnerDepth() {
    }

    void mix_exchanges(const SInnerDepth& src, double bias, uint32 kind=1) 
    {
        if (kind == 1 && bias > -100)
        {
            for( const auto& v : src.exchanges ) 
            {                
                exchanges[v.first] += (v.second * (1 + bias)) > 0 ? (v.second * (1 + bias)) : 0;
            }
        }
        else if (kind == 2)
        {
            for( const auto& v : src.exchanges ) 
            {                
                exchanges[v.first] += (v.second + bias) > 0 ? (v.second + bias) :0;
            }
        }



        total_volume = 0;
        for( const auto& v : exchanges ) {
            total_volume += v.second;
        }
    }

    void set_total_volume()
    {
        total_volume = 0;
        for( const auto& v : exchanges ) {
            total_volume += v.second;
        }        
    }
};

struct HedgeInfo
{
    HedgeInfo(string symbol_value, double price_value, double amount_value, 
            TradedOrderStreamData_Direction direction_value, bool is_trade):
            symbol{symbol_value}
            {
                cout << symbol << " ";
                if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_BUY)
                {
                    
                    ask_amount = amount_value;

                    cout << "set ask_amount: "  << ask_amount << endl;
                }
                else if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_SELL)
                {
                    
                    bid_amount = amount_value;

                    cout << "set bid_amount: "  << bid_amount << endl;
                }
                else
                {
                    cout << "HedgeInfo Unknow Direction!" << endl;
                }

            }
    
    HedgeInfo()
    {

    }

    void set(string symbol_value, double price_value, double amount_value, 
            TradedOrderStreamData_Direction direction_value, bool is_trade)
    {
        symbol = symbol_value;

        cout << symbol << " ";
        if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_BUY)
        {
            // if (is_trade)
            // {
            //     cout << "Minus ask_amount " << ask_amount << ", " << amount_value << endl;

            //     ask_amount -= amount_value;
                
            // }
            // else
            // {
            //     cout << "Add ask_amount " << ask_amount << ", " << amount_value << endl;

            //     ask_amount += amount_value;
                
            // }          

            ask_amount = amount_value;  
        }
        else if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_SELL)
        {
            // if (is_trade)
            // {
            //     cout << "Minus bid_amount " << bid_amount << ", " << amount_value << endl;

            //     bid_amount -= amount_value;
            // }
            // else
            // {
            //     cout << "Add bid_amount " << bid_amount << ", " << amount_value << endl;
            //     bid_amount += amount_value;
            // }

            bid_amount = amount_value;
        }
        else
        {
            cout << "HedgeInfo Set Unknow Direction!" << endl;
        }        


    }

    string symbol;
    double ask_amount{0};
    double bid_amount{0};
};

struct SInnerQuote {
    string exchange;
    string symbol;
    type_tick time_origin;      // 交易所原始时间
    type_tick time_arrive_at_streamengine;   // se收到的时间
    type_tick time_produced_by_streamengine;    // se处理完发送的时间
    type_tick time_arrive;  // rc收到的时间
    type_seqno seq_no;
    uint32 precise;
    uint32 vprecise;
    map<SDecimal, SInnerDepth> asks;
    map<SDecimal, SInnerDepth> bids;

    SInnerQuote() {
        seq_no = 0;
        precise = 0;
        vprecise = 0;
        time_origin = time_arrive_at_streamengine = time_produced_by_streamengine = time_arrive = 0;
    }

    void get_asks(vector<pair<SDecimal, SInnerDepth>>& depths) const {
        depths.clear();
        for( auto iter = asks.begin() ; iter != asks.end() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }

    void get_bids(vector<pair<SDecimal, SInnerDepth>>& depths) const {
        depths.clear();
        for( auto iter = bids.rbegin() ; iter != bids.rend() ; iter ++ ) {
            depths.push_back(make_pair(iter->first, iter->second));
        }
    }
};

class CallDataServeMarketStream;


struct Params {
    AccountInfo cache_account;
    map<TSymbol, QuoteConfiguration> cache_config;
    map<TSymbol, SymbolConfiguration> symbol_config;
    map<TSymbol, HedgeInfo> hedage_info;
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

    SInnerQuote& run(SInnerQuote& src, PipelineContent& ctx) {
        if( next_ ) {
            SInnerQuote& tmp = this->process(src, ctx);
            return next_->run(tmp, ctx);
        } else {
            return this->process(src, ctx);
        }
    }

    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx) = 0;
private:
    Worker *next_ = nullptr;
};

class DefaultWorker : public Worker
{
private:
    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx);
};

class QuotePipeline
{
public:
    QuotePipeline();
    virtual ~QuotePipeline();

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

        assert( head_ != NULL );
        
        SInnerQuote tmp = quote;
        newQuote = head_->run(tmp, ctx);
    }
private:
    Worker *head_ = nullptr, *tail_ = nullptr;

    DefaultWorker default_worker_;
};

// worker：处理订单簿风控
class QuoteBiasWorker : public Worker
{
private:
    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx);
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
    void query(map<TSymbol, SDecimal>& watermarks) const;
    void _calc_watermark();

private:
    // 独立线程计算watermark
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, SymbolWatermark*> watermark_;
    std::thread* thread_loop_ = nullptr;
    std::atomic<bool> thread_run_;
    void thread_func();

    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx);
};

// worker：处理对冲账户资金量风控
class AccountAjdustWorker: public Worker
{
public:
    void set_snap(const SInnerQuote& quote);
private:
    // cache
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, int> currency_count_;
    unordered_set<TSymbol> symbols_;
    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx);

    bool get_currency(const SInnerQuote& quote, string& sell_currency, int& sell_count, string& buy_currency, int& buy_count) const;
};

// worker：处理订单簿风控
class OrderBookWorker : public Worker
{
private:
    virtual SInnerQuote& process(SInnerQuote& src, PipelineContent& ctx);
};

/*
缓存接口
*/
class IDataCacher
{
public:
    // 询价查询
    // 返回0 执行成功 
    // 返回1 没有找到币对
    virtual QuoteResponse_Result otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double volume, double amount, SDecimal& price) = 0;

    virtual void hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade) = 0;

    // 查询内部参数
    virtual void get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations) = 0;

    // 查询快照
    virtual bool get_snaps(vector<SInnerQuote>& snaps) = 0;
};

class IQuotePusher
{
public:
    virtual void publish4Broker(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update) = 0;
    virtual void publish4Hedge(const string& symbol, std::shared_ptr<MarketStreamData> snap, std::shared_ptr<MarketStreamData> update) = 0;
    virtual void publish4Client(const string& symbol, std::shared_ptr<MarketStreamDataWithDecimal> snap, std::shared_ptr<MarketStreamDataWithDecimal> update) = 0;
};

class DataCenter : public IDataCacher 
{
public:
    DataCenter();
    ~DataCenter();

    // 回调行情通知
    void add_quote(const SInnerQuote& quote);
    // 触发重新计算，并下发行情给所有client
    void change_account(const AccountInfo& info);
    // 触发重新计算，并下发行情给所有client
    void change_configuration(const map<TSymbol, QuoteConfiguration>& config);

    void change_configuration(const map<TSymbol, SymbolConfiguration>& config);

    // 触发指定品种重新计算，并下发该品种行情给所有client
    void change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids);
    // 询价查询
    QuoteResponse_Result otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double volume, double amount, SDecimal& price);
    // 注册推送接口
    void register_callback(IQuotePusher* callback) { callbacks_.insert(callback); }

    bool get_snaps(vector<SInnerQuote>& snaps);

    void get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations);

    bool check_quote(SInnerQuote& quote);

    virtual void hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade);

private:
    set<IQuotePusher*> callbacks_;

    void _publish_quote(const SInnerQuote& quote);

    void _push_to_clients(const string& symbol = "");

    mutable std::mutex                  mutex_datas_;
    unordered_map<TSymbol, SInnerQuote> datas_;
    unordered_map<TSymbol, SInnerQuote> last_datas_;
    Params params_;



    // 处理流水线
    QuotePipeline pipeline_;
    QuoteBiasWorker quotebias_worker_;
    WatermarkComputerWorker watermark_worker_;
    AccountAjdustWorker account_worker_;
    OrderBookWorker orderbook_worker_;
};