#pragma once
#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "comm_interface_define.h"
#include "comm.h"
#include "risk_interface_define.h"
#include "native_config.h"
#include "riskcontrol_worker.h"

#include "util/tool.h"
#include "global_declare.h"


// 流水线模型
struct PipelineContent
{
    Params params;
    bool is_sample_;
};

class CallDataServeMarketStream;

class Worker
{
public:
    Worker() {}
    virtual ~Worker() {}

    void add_worker(Worker* w) {
        this->next_ = w;
    }

    // SDepthQuote& run(SDepthQuote& src, PipelineContent& ctx) {
    //     if( next_ ) {
    //         SDepthQuote& tmp = this->process(src, ctx);
    //         return next_->run(tmp, ctx);
    //     } else {
    //         return this->process(src, ctx);
    //     }
    // }


    SDepthQuote& run(SDepthQuote& src, PipelineContent& ctx) {

        if (filter_zero_volume(src))
        {
            LOG_WARN("\nBefore "+ worker_name + " " + src.symbol + quote_str(src));
        }  

        SDepthQuote& tmp = this->process(src, ctx);

        if (tmp.asks.size() == 0 && tmp.bids.size() == 0)
        {
            return tmp;
        }

        if (filter_zero_volume(src))
        {
            LOG_WARN("\nAfter "+ worker_name + " " + src.symbol + quote_str(src));
        }          

        if( next_ ) {            
            return next_->run(tmp, ctx);
        } else {
            return tmp;
        }
    }


    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx) = 0;

    string worker_name{""};
private:
    Worker *next_ = nullptr;
    std::mutex      filter_quote_mutex_;
};

class DefaultWorker : public Worker
{
public:
    DefaultWorker() { worker_name = "DefaultWorker";}
private:
    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);
};

class PrecisionWorker : public Worker
{
public:
    PrecisionWorker() { worker_name = "PrecisionWorker";}
private:
    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);
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

    void run(const SDepthQuote& quote, const Params& params, SDepthQuote& newQuote) {
        PipelineContent ctx;
        ctx.params = params;
        if( quote.symbol == NATIVE_CONFIG->sample_symbol_ ) {
            ctx.is_sample_ = true;
        } else {
            ctx.is_sample_ = false;
        }

        assert( head_ != NULL );
        
        SDepthQuote tmp = quote;

        
        newQuote = head_->run(tmp, ctx);
    }
private:
    Worker *head_ = nullptr, *tail_ = nullptr;

    DefaultWorker default_worker_;
};

// worker：处理订单簿风控
class QuoteBiasWorker : public Worker
{
public:
    QuoteBiasWorker() { worker_name = "QuoteBiasWorker";}
private:
    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);
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
    void set_snap(const SDepthQuote& quote);
    void query(map<TSymbol, SDecimal>& watermarks) const;
    void _calc_watermark();

private:
    // 独立线程计算watermark
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, SymbolWatermark*> watermark_;
    std::thread* thread_loop_ = nullptr;
    std::atomic<bool> thread_run_;
    void thread_func();

    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);
};

// worker：处理对冲账户资金量风控
class AccountAjdustWorker: public Worker
{
public:
    AccountAjdustWorker() { worker_name = "AccountAjdustWorker";}
    void set_snap(const SDepthQuote& quote);
private:
    // cache
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, int> currency_count_;
    unordered_set<TSymbol> symbols_;
    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);

    bool get_currency(const SDepthQuote& quote, string& sell_currency, int& sell_count, string& buy_currency, int& buy_count) const;
};

// worker：处理订单簿风控
class OrderBookWorker : public Worker
{
    public:
    OrderBookWorker() { worker_name = "OrderBookWorker";}
private:
    virtual SDepthQuote& process(SDepthQuote& src, PipelineContent& ctx);
};
