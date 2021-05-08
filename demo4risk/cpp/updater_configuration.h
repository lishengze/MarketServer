#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/nacos_client.h"
#include "risk_controller_define.h"

#include "pandora/util/thread_safe_singleton.hpp"
#include <mutex>
#include <sstream>

#include <string>
using std::string;

struct QuoteConfiguration
{
    //double MakerFee; // 百分比；maker手续费
    //double TakerFee; // 百分比；taker手续费
    std::string symbol;
    uint32 PublishFrequency;
    uint32 PublishLevel;
    uint32 PriceOffsetKind;  // 价格偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double PriceOffset;      // 百分比；价格偏移量（买卖盘相同）

    uint32 AmountOffsetKind; // 数量偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double AmountOffset;     // 行情数量偏移

    double DepositFundRatio; // 充值资金动用比例

    //double UserPercent;    // 百分比；用户账户可动用比例
    double HedgeFundRatio;   // 百分比；对冲账户可动用比例

    uint32 OTCOffsetKind;    // 询价偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.

    double OtcOffset;        // 询价偏移
    bool   IsPublish{true};

    std::string desc() const {

        stringstream s_obj;
        s_obj << "symbol: " << symbol << "\n"
              << "PublishFrequency: " << PublishFrequency << "\n"
              << "PublishLevel: " << PublishLevel << "\n"
              << "PriceOffsetKind: " << PriceOffsetKind << "\n"
              << "PriceOffset: " << PriceOffset << "\n"
              << "AmountOffsetKind: " << AmountOffsetKind << "\n"
              << "AmountOffset: " << AmountOffset << "\n"
              << "DepositFundRatio: " << DepositFundRatio << "\n"
              << "HedgeFundRatio: " << HedgeFundRatio << "\n"
              << "OTCOffsetKind: " << OTCOffsetKind << "\n"
              << "OtcOffset: " << OtcOffset << "\n"
              << "IsPublish: " << IsPublish << "\n";
        
        return s_obj.str();
    }
};

struct SymbolConfiguration {
  std::string SymbolId;           // `json:"symbol_id"` //品种代码，如 BTC_USDT
  std::string SymbolKind;         //`json:"symbol_kind"` //品种类型，如 现货、期货等
  std::string Bid;                // `json:"bid"` //标的，如 BTC
  std::string PrimaryCurrency;    //  `json:"primary_currency"` //基础货币，如 USDT
  std::string BidCurrency;       // `json:"bid_currency"` //报价货币，如 USDT
  std::string SettleCurrency;     //   `json:"settle_currency"` //结算货币，如 USDT
  bool Switch;                  // `json:"switch"` //交易开关
  int AmountPrecision;         //`json:"amount_precision"` //数量精度
  int PricePrecision;       //json:"price_precision"` //价格精度
  int SumPrecision;         //`json:"sum_precision"` //金额精度
  double MinUnit;            //`json:"min_unit"` //最小交易单位
  double MinChangePrice;    //`json:"min_change_price"` //最小变动价位
  double Spread;            //`json:"spread"` //点差，品种tick值的整数倍
  int    FeeKind;        //`json:"fee_kind"` //手续费算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
  double TakerFee;      //`json:"taker_fee"` //Taker手续费率
  double MakerFee;      //`json:"maker_fee"` //Maker手续费率
  double MinOrder;      //`json:"min_order"` //单次最小下单量
  double MaxOrder;    //`json:"max_order"` //单次最大下单量
  double MinMoney;    //      `json:"min_money"` //单次最小下单金额
  double MaxMoney;     // float64    `json:"max_money"` //单次最大下单金额
  double BuyPriceLimit;    //float64    `json:"buy_price_limit"` //买委托价格限制
  double SellPriceLimit;  //float64    `json:"sell_price_limit"` //卖委托价格限制
  int MaxMatchLevel;         //`json:"max_match_level"` //最大成交档位,不得超过20
  double OtcMinOrder; //`json:"otc_min_order"` //OTC最小量
  double OtcMaxOrder;    //`json:"otc_max_order"` //OTC最大量
  double OtcMinPrice;     //`json:"otc_min_price"` //OTC最小金额
  double OtcMaxPrice;     //`json:"otc_max_price"` //OTC最大金额
  std::string User;       //`json:"user"` //操作员
  std::string Time;          //`json:"time"` //操作时间

    std::string desc() const {
        stringstream s_obj;
        s_obj << "SymbolId: " << SymbolId << "\n"
              << "SymbolKind: " << SymbolKind << "\n"
              << "Bid: " << Bid << "\n"
              << "PrimaryCurrency: " << PrimaryCurrency << "\n"
              << "BidCurrency: " << BidCurrency << "\n"
              << "SettleCurrency: " << SettleCurrency << "\n"
              << "Switch: " << Switch << "\n"
              << "AmountPrecision: " << AmountPrecision << "\n"
              << "PricePrecision: " << PricePrecision << "\n"
              << "SumPrecision: " << SumPrecision << "\n"
              << "MinUnit: " << MinUnit << "\n"
              << "MinChangePrice: " << MinChangePrice << "\n"

              << "Spread: " << Spread << "\n"
              << "FeeKind: " << FeeKind << "\n"
              << "TakerFee: " << TakerFee << "\n"
              << "MakerFee: " << MakerFee << "\n"
              << "MinOrder: " << MinOrder << "\n"
              << "MaxOrder: " << MaxOrder << "\n"
              << "MinMoney: " << MinMoney << "\n"
              << "MaxMoney: " << MaxMoney << "\n"
              << "MaxMatchLevel: " << MaxMatchLevel << "\n"
              << "OtcMinOrder: " << OtcMinOrder << "\n"
              << "OtcMaxOrder: " << OtcMaxOrder << "\n"

              << "OtcMinPrice: " << OtcMinPrice << "\n"
              << "OtcMaxPrice: " << OtcMaxPrice << "\n"
              << "User: " << User << "\n"
              << "Time: " << Time << "\n";                           

        return s_obj.str();
    }  
};

class IConfigurationUpdater {
public:
    virtual void on_configuration_update(const map<TSymbol, QuoteConfiguration>& config) = 0;

    virtual void on_configuration_update(const map<TSymbol, SymbolConfiguration>& config) = 0;
};

class ConfigurationClient: public NacosClient
{
public:
    void set_callback(IConfigurationUpdater* callback) { callback_ = callback; }
    
    // derive from NacosClient
    void config_changed(const std::string& group, const std::string& dataid, const NacosString &configInfo);

    void load_symbol_params(const NacosString &configInfo);

    bool check_symbol(std::string symbol);

    // map<TSymbol, QuoteConfiguration>& get_risk_config() { return risk_config_;}


private:
    // derive from NacosClient
    void _run();

    IConfigurationUpdater* callback_ = nullptr;

    // NacosListener
    NacosListener risk_watcher_;

    NacosListener symbol_watcher_;
    
    SymbolConfiguration symbol_params_;

    // 处理配置数据
    void _parse_config();
    NacosString risk_params_; // for 品种更新频率 和 品种更新深度

    // map<TSymbol, QuoteConfiguration> risk_config_;
    // std::mutex                       risk_config_mutex_;
};

#define RISK_CONFIG utrade::pandora::ThreadSafeSingleton<ConfigurationClient>::DoubleCheckInstance() 
