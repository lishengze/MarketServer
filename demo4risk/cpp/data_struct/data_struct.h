#pragma once

#include "../global_declare.h"
#include "../Log/log.h"

#include "stream_engine.grpc.pb.h"
#include "stream_engine.pb.h"

#include "risk_controller.grpc.pb.h"
#include "risk_controller.pb.h"

#include "account.grpc.pb.h"
#include "account.pb.h"

#include "quote_data.pb.h"


struct SOrder {
    long long order_id; // 系统内的订单唯一ID
    SDecimal price; // 下单价格
    double volume;  // 下单量
    bool is_buy; // 买卖方向
    long long begin_time; // 对冲开始时间
    long long end_time; // 对冲完成时间

    SOrder() {
        volume = 0;
        begin_time = 0;
        end_time = 0;
    }
};

struct SOrderPriceLevel {
    SDecimal price;
    double volume;

    SOrderPriceLevel() {
        volume = 0;    
    }
};

struct CurrencyInfo {
    double amount;

    std::string str()
    {
        return std::to_string(amount);
    }
};

struct HedgeAccountInfo
{
    unordered_map<TSymbol, CurrencyInfo> currencies;

    HedgeAccountInfo() {
    }

    std::string str()
    {
        if (currencies.size() == 0) return "";
        std::stringstream s_s;
        for (auto iter:currencies)
        {
            s_s << iter.first << ": " << iter.second.str() << "\n";
        }
        return s_s.str();
    }
};

struct UserAccountInfo
{
    unordered_map<TSymbol, CurrencyInfo> currencies;

    UserAccountInfo() {
    }
};

/*
HedgeParams

    "platform_id":"HUOBI",
    "instrument":"BTC_USDT",
    "switch":true,
    "buy_fund_ratio":0,
    "sell_fund_ratio":0,
    "price_precision":2,
    "amount_precision":4,
    "sum_precision":2,
    "min_unit":0.0001,
    "min_change_price":0.01,
    "max_margin":1,
    "max_order":100,
    "buy_price_limit":10000,
    "sell_price_limit":5000,
    "max_match_level":5,
    "fee_kind":1,
    "taker_fee":0.0020,
    "maker_fee":0.0020
*/
struct HedgeConfig
{
    //double MakerFee; // 百分比；maker手续费
    //double TakerFee; // 百分比；taker手续费
    std::string symbol;
    std::string exchange;

    double BuyFundPercent;
    double SellFundPercent;

    std::string str() const {

        stringstream s_obj;
        s_obj << "symbol: " << symbol << ","
              << "exchange: " << exchange << ","
              << "BuyFundPercent: " << BuyFundPercent << ","
              << "SellFundPercent: " << SellFundPercent << "\n";
        
        return s_obj.str();
    }
};

struct AccountInfo
{
    unordered_map<TExchange, HedgeAccountInfo> hedge_accounts_;
    UserAccountInfo user_account_;

    double get_user_amount(const string& currency) const {
        return 0;
    }
    
    double get_hedge_amount(const string& currency) const {
        double total = 0;
        for( auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) {
            const HedgeAccountInfo& hedge = iter->second;
            auto iter2 = hedge.currencies.find(currency);
            if( iter2 != hedge.currencies.end() ) {
                total += iter2->second.amount;
            }
        }
        return total;
    }

    void get_hedge_amounts(const string& currency, 
                            map<TExchange, HedgeConfig>& hedge_config, 
                            unordered_map<TExchange, double>& amounts, 
                            bool is_buy) const 
    {
        try 
        {
            for(auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) 
            {
                TExchange exchange = iter->first;                
                const HedgeAccountInfo& hedge = iter->second;

                if (hedge.currencies.find(currency) == hedge.currencies.end())
                {
                    LOG_WARN("hedge.currencies does not have currency: " + currency);
                    continue;
                }

                if (hedge_config.find(exchange) == hedge_config.end())
                {
                    LOG_WARN("hedge_config does not have exchange: " + exchange);
                    continue;
                }

                const CurrencyInfo& currency_info = hedge.currencies.find(currency)->second;

                if (is_buy)
                {
                    amounts[exchange] = currency_info.amount * hedge_config[exchange].BuyFundPercent;
                }
                else
                {
                    amounts[exchange] = currency_info.amount * hedge_config[exchange].SellFundPercent;
                }
            }
        }
        catch(const std::exception& e)
        {
            LOG_ERROR(e.what());
        }


    }

    string hedage_account_str()
    {
        if (hedge_accounts_.size() == 0) return "";
        std::stringstream s_s;
        s_s << "\n";
        for (auto iter:hedge_accounts_)
        {
            s_s << iter.first << ", " << iter.second.str() << "\n";
        }
        return s_s.str();
    }



};

/*


{
    "symbol_id":"BTC_USDT",
    "switch":true,
    "publish_frequency":1,
    "publish_level":20,
    "price_offset_kind":1,
    "price_offset":0.001,
    "amount_offset_kind":1,
    "amount_offset":0.5,
    "poll_offset_kind":1,
    "poll_offset":0.001,
    "user":"admin",
    time":"2021-07-13 06:25:33"},
*/
struct MarketRiskConfig
{
    //double MakerFee; // 百分比；maker手续费
    //double TakerFee; // 百分比；taker手续费
    std::string symbol;

    uint32 PublishFrequency; // 发布频率
    uint32 PublishLevel;
    uint32 PriceOffsetKind;  // 价格偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double PriceOffset;      // 百分比；价格偏移量（买卖盘相同）

    uint32 AmountOffsetKind; // 数量偏移算法，取值1或2，1表示百比分，2表示绝对值。默认为1.
    double AmountOffset;     // 行情数量偏移

    double DepositFundRatio; // 充值资金动用比例

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

    std::string fee_info() const
    {
        stringstream s_obj;
        s_obj << "SymbolId: " << SymbolId << "\n"
              << "SymbolKind: " << SymbolKind << "\n"
              << "FeeKind: " << FeeKind << "\n"
              << "TakerFee: " << TakerFee << "\n"
              << "MakerFee: " << MakerFee << "\n";                           

        return s_obj.str();        
    }
};

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
        LOG_INFO("HedgeInfo Init " + symbol);
        if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_BUY)
        {
            
            ask_amount = amount_value;

           LOG_INFO("init ask_amount: " + std::to_string(ask_amount));
        }
        else if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_SELL)
        {
            
            bid_amount = amount_value;

            LOG_INFO("init bid_amount: " + std::to_string(bid_amount));
        }
        else
        {
            LOG_WARN("HedgeInfo Set Unknow Direction!");
        }

    }
    
    HedgeInfo()
    {

    }

    void set(string symbol_value, double price_value, double amount_value, 
            TradedOrderStreamData_Direction direction_value, bool is_trade)
    {
        symbol = symbol_value;

        LOG_INFO("HedgeInfo Set " + symbol);
        if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_BUY)
        {     
            ask_amount = amount_value;  

            LOG_INFO("new ask_amount: " + std::to_string(ask_amount));
        }
        else if (direction_value == TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_SELL)
        {

            bid_amount = amount_value;

            LOG_INFO("new bid_amount: " + std::to_string(bid_amount));
        }
        else
        {
            LOG_WARN("HedgeInfo Set Unknow Direction!");
        }        


    }

    string str()
    {
        return symbol + ", ask_amount: " +std::to_string(ask_amount) + ", bid_amount: " + std::to_string(bid_amount);
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

struct Params {
    AccountInfo account_config;
    map<TSymbol, MarketRiskConfig> quote_config;
    map<TSymbol, SymbolConfiguration> symbol_config;
    map<TSymbol, HedgeInfo> hedage_order_info;
    map<TSymbol, map<TExchange, HedgeConfig>> hedge_config;
    unordered_map<TSymbol, pair<vector<SOrderPriceLevel>, vector<SOrderPriceLevel>>> cache_order;
};