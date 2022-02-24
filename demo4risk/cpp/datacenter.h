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
    // 触发重新计算，并下发行情给所有client
    void change_account(const AccountInfo& info);
    // 触发重新计算，并下发行情给所有client
    void change_configuration(const map<TSymbol, MarketRiskConfig>& config);

    void change_configuration(const map<TSymbol, SymbolConfiguration>& config);

    void change_configuration(const map<TSymbol, map<TExchange, HedgeConfig>>& config);

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


    void precheck_quote(const SInnerQuote& quote);

    void set_src_quote(const SInnerQuote& quote);

    bool process(const SInnerQuote& quote);

private:
    set<IQuotePusher*> callbacks_;

    void _publish_quote(const SInnerQuote& quote);

    void _push_to_clients(const string& symbol = "");

    mutable std::mutex                  mutex_datas_;
    mutable std::mutex                  mutex_config_;

    unordered_map<TSymbol, SInnerQuote> datas_;
    unordered_map<TSymbol, SInnerQuote> last_datas_;
    Params params_;

    // 处理流水线
    QuotePipeline           pipeline_;

    FeeWorker               fee_worker_;
    AccountAjdustWorker     account_worker_;
    OrderBookWorker         orderbook_worker_;
    QuoteBiasWorker         quotebias_worker_;
    WatermarkComputerWorker watermark_worker_;
    PrecisionWorker         pricesion_worker_;


private:
    void start_check_symbol();
    void set_check_symbol_map(TSymbol symbol);
    void check_symbol_main();

    void check_symbol();

    void erase_outdate_symbol(TSymbol symbol);
    std::thread             check_thread_;
    std::mutex              check_symbol_mutex_;

    map<TSymbol, int>       check_symbol_map_;


};