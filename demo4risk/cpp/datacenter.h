#pragma once

#include "riskcontrol_worker.h"

#include "updater_configuration.h"
#include "updater_account.h"
#include "updater_order.h"
#include "risk_controller_define.h"
#include "grpc_entity.h"

#include "Log/log.h"

#include "data_struct/data_struct.h"
#include "util/tool.h"
#include "Log/log.h"
#include "base/cpp/base_data_stuct.h"


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
    void add_quote(SInnerQuote& quote);

    void update_trade(const TradeData& trade);

    double get_price(const string& symbol);

    double get_offset(double amount, const MarketRiskConfig& config);

    // 触发重新计算，并下发行情给所有client
    void change_account(const AccountInfo& info);
    // 触发重新计算，并下发行情给所有client
    void change_configuration(const map<TSymbol, MarketRiskConfig>& config);

    void change_configuration(const map<TSymbol, SymbolConfiguration>& config);

    void change_configuration(const map<TSymbol, map<TExchange, HedgeConfig>>& config);

    // 触发指定品种重新计算，并下发该品种行情给所有client
    void change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids);
    
    QuoteResponse_Result _calc_otc_by_volume(const map<SDecimal, SInnerDepth>& depths, bool is_ask, MarketRiskConfig& config, double volume, SDecimal& dst_price, uint32 precise);

    QuoteResponse_Result _calc_otc_by_amount(const map<SDecimal, SInnerDepth>& depths, bool is_ask, MarketRiskConfig& config, double otc_amount, SDecimal& dst_price, uint32 precise);

    // 询价查询
    QuoteResponse_Result otc_query(const TExchange& exchange, const TSymbol& symbol, QuoteRequest_Direction direction, double volume, double amount, SDecimal& price);
    // 注册推送接口
    void register_callback(IQuotePusher* callback) { callbacks_.insert(callback); }

    bool get_snaps(vector<SInnerQuote>& snaps);

    void get_params(map<TSymbol, SDecimal>& watermarks, map<TExchange, map<TSymbol, double>>& accounts, map<TSymbol, string>& configurations);

    virtual void hedge_trade_order(string& symbol, double price, double amount, TradedOrderStreamData_Direction direction, bool is_trade);

    void process_symbols(std::list<string> symbol_list);

    bool process(const SInnerQuote& quote);

    void precheck_quote(const SInnerQuote& quote);

    void set_src_quote(const SInnerQuote& quote);

    bool check_quote_time(const SInnerQuote& src_quote, const SInnerQuote& processed_quote);

    bool check_quote_publish(SInnerQuote& quote);

    double get_usd_price(const string& symbol);

private:
    set<IQuotePusher*> callbacks_;

    void _update_riskctrl_data(const SInnerQuote& quote);

    void _publish_quote(const SInnerQuote& quote);

    void _push_to_clients(const string& symbol = "");

    
    mutable std::mutex                  mutex_config_;

    unordered_map<TSymbol, SInnerQuote> original_datas_;
    mutable std::mutex                  mutex_original_datas_;

    unordered_map<TSymbol, SInnerQuote> riskctrl_datas_;
    mutable std::mutex                  mutex_riskctrl_datas_;

    unordered_map<TSymbol, TradeData>   trade_data_map_;
    mutable std::mutex                  trade_data_mutex_;

    Params params_;

    // 处理流水线
    QuotePipeline               riskctrl_work_line_;
    FeeWorker                   fee_worker_;
    AccountAjdustWorker         account_worker_;
    OrderBookWorker             orderbook_worker_;
    QuoteBiasWorker             quotebias_worker_;
    WatermarkComputerWorker     watermark_worker_;
    PrecisionWorker             pricesion_worker_;


private:
    void start_check_symbol();
    void set_check_symbol_map(TSymbol symbol);
    void check_symbol_main();

    void check_symbol();

    void erase_outdate_symbol(TSymbol symbol);
    std::thread             check_thread_;
    std::mutex              check_symbol_mutex_;

    map<TSymbol, int>       check_symbol_map_;

private:

    void start_publish();
    void publish_main();
    void update_publish_data_map();
    int  get_sleep_millsecs(unsigned long long cur_time, int fre);
    int  get_sleep_millsecs(unsigned long long time, std::list<string>& publish_list);

    struct FreqData
    {
        int     freq_millsecs{1000};
        int     sleep_milleses{1000};
        string  next_symbol;
    };

    string                       cur_symbol;
    std::map<string, int>        frequency_map_;
    std::mutex                   mutex_frequency_map_;

    std::thread                  publish_thread_;


};