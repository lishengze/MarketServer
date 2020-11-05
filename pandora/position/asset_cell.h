#pragma once
#include "../pandora_declare.h"
#include "../messager/messager.h"
#include "quark/cxx/ut/UtData.h"
#include "quark/cxx/assign.h"

class CUTRtnAccountField;
class CUTRtnPositionField;

PANDORA_NAMESPACE_START

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position cell and currency cell base general interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IAssetCell : public IMessager, public IRedisPublisher
{
public:
    IAssetCell();
    virtual ~IAssetCell() {}
    // retrieve the frozen buy amount
    virtual double frozen_buy() = 0;
    // retrieve the frozen sell amount
    virtual double frozen_sell() = 0;
    // retrieve the available amount
    virtual double available() = 0;
    // retrieve current hold amount
    virtual double hold() = 0;

    // account name that the currency belong to
    virtual string account_name() = 0;
    // account type, in galaxy system: physical, logical, channel, strategy
    virtual const char& account_type() const = 0;
    // asset type: such as digitalspot, digitalfuture, future etc.
    virtual const char& asset_type() = 0;
    // currency identification, unique (对于持仓而言代表绑定在那个币种上进行的交易，合约总是建立在某个基础货币上进行交易)
    virtual long currency_id() = 0;

    // set the exchange name of this currency belonged to, but maybe the currency is mixed.
    virtual void set_exchange_id(const string& exchangeid) = 0; 

    // quote check: != 0
    bool quote_checker(double quota, const string& tips);
    // check if the subtrahend if great minuend
    bool great_checker(double subtrahend, double minuend, const string& tips_1, const string& tips_2);


protected:
    // init hold asset quota (初始配额)
    double hold_init_{0.0};
    // hold offset （动态调整额度）
    double hold_offset_{0.0};
};
DECLARE_PTR(IAssetCell);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position cell here general interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPositionCell : public IAssetCell
{
public:
    IPositionCell(const std::string& account_name, const CAccountTypeType& account_type, const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid, const CPosiDirectionType& direction);
    virtual ~IPositionCell(){}

    // frozen
    virtual bool frozen(const double& quota, const double& currency_quota) = 0;
    // unfrozen
    virtual void unfrozen(const double& quota, const double& currency_quota) = 0;
    // confirm
    virtual void confirm(const double& confirm_quota, const double& unfrozen_quota, const double& confirm_currency_quota, const double& unfrozen_currency_quota) = 0;

    // bind currency id
    void bind_currency_id(long currency_id) { assign(position_->CurrencyID, currency_id); }
    // init position
    void init_position(const CUTRtnPositionField& position);

    // retrieve the capital name
    // return account name that the currency belong to
    std::string instrument_id(){ return std::string(position_->InstrumentID); }
    // position direction
    const char position_direction() { return position_->PosiDirection; }
    // return　position average price
    double avg_price() { return position_->Price; }
    // last update price
    double last_price() { return last_price_; }
    // returive position
    double position() { return hold(); }
    // order occupy the margin
    virtual double order_margin() { return position_->OrderMargin; }
    // occupy the margin
    virtual double position_margin() { return position_->PositionMargin; }
    // set the last price
    virtual void set_last_price(const double& price) { last_price_ = price; }
    // set the avg price
    virtual void set_avg_price(const double& price) { assign(position_->Price, price); }
    // return frozen buy amount
    virtual double frozen_buy() override { return position_->FrozenBuy; }
    // return frozen sell amount
    virtual double frozen_sell() override { return position_->FrozenSell; }
    // return account name that the currency belong to
    virtual std::string account_name() override { return std::string(position_->AccountName); }
    // account type, in galaxy system: physical, logical, channel, strategy
    const char& account_type() const { return position_->AccountType; }
    // exchange identification
    // asset type: such as digitalspot, digitalfuture, future etc.
    const char& asset_type() { return position_->AssetType; }
    // currency identification, unique
    long currency_id() { return position_->CurrencyID; }
    
    // 获取浮动盈亏
    virtual double float_profit() = 0;
    // 获取持仓的字段信息
    boost::shared_ptr<CUTRtnPositionField> position_field();
    // 是否传统计算方法
    void set_settle(bool classic) { classic_settle_ = classic; }
    // 设定交易所名称
    virtual void set_exchange_id(const string& exchangeid) override { assign(position_->ExchangeID, exchangeid.c_str()); }

protected:
    // last price
    double last_price_{0.0};
    // classic settle
    bool classic_settle_{true};
    // wrapped position detail
    boost::shared_ptr<CUTRtnPositionField> position_;
};
DECLARE_PTR(IPositionCell);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// currency in accout general interface    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ICurrencyCell : public IAssetCell
{
public:
    ICurrencyCell(const string& account_name, const CAccountTypeType& account_type, const string& currency_name, const CAssetTypeType& asset_type);
    virtual ~ICurrencyCell(){}

    // solve frozen, unfrozen, confirm
    // invoke by create order
    virtual bool frozen(const double& quota) = 0;
    // invoke by riskctrl or cancel order
    virtual bool unfrozen(const double& quota) = 0;
    // invoke by tradecapital_name
    virtual bool confirm(const double& quota_trade, const double& quota_unfrozen) = 0;

    virtual bool confirm_for_transact(const double& quota_trade, const double& quota_unfrozen) = 0;
    // invoke by create margin order
    virtual bool frozen_margin(const double& quota) = 0;
    // invoke by cancel margin order
    virtual void unfrozen_margin(const double& quota) = 0;
    // invoke by margin trade
    virtual void confirm_margin(const double& quota_trade, const double& quota_unfrozen) = 0;

    // return frozen buy amount
    virtual double frozen_buy() override { return account_->FrozenBuy; }
    // return frozen sell amount
    virtual double frozen_sell() override { return account_->FrozenSell; }
    // return account name that the currency belong to
    virtual std::string account_name() override { return std::string(account_->AccountName); }
    // retrieve the capital name
    string currency_name() { return std::string(account_->CurrencyName); }
    // account type, in galaxy system: physical, logical, channel, strategy
    const char& account_type() const { return account_->AccountType; }
    // exchange identification
    // const string& exchange_id() const { return account_->ExchangeID; }
    // const char& operate_direction() const { return operate_direction_; }
    // asset type: such as digitalspot, digitalfuture, future etc.
    const char& asset_type() { return account_->AssetType; }
    // currency identification, unique
    long currency_id() { return account_->CurrencyID; }

    // 考虑到浮动盈亏与权益计算关乎持仓，所以这里对持仓进行绑定
    // 严重注意：目前的这种方法只针对账户中的一个合约只能用一种币进行买卖，当某个合约可用使用多个币进行买卖就会出问题(因复杂的逻辑账户)
    // 因为绑定的 position 有可能是别的币种买的，不一定是当前币种．如果需要彻底解决这个问题，需要在当前币种中自己管理独有的币种对应持仓
    void bind_position(IPositionCellPtr position, const string& symbol, const char& direction=PD_Net);

    // interface to retrieve position, 
    // @symbol specific the symbol
    // @direction specific the direction, default by PD_Net
    IPositionCellPtr retrieve_position(const string& symbol, const CPosiDirectionType& direction=PD_Net);
    // retrieve all positions
    std::unordered_map<string, IPositionCellPtr>& retrieve_positions() { return positions_;}

    // init position
    virtual void init_currency(const CUTRtnAccountField& account);
    // position margin
    double position_margin() { return account_->PositionMargin; }
    // get the order margin
    double order_margin() { return account_->OrderMargin; }
    // get the fee amount
    virtual double fee() { return account_->Fee; }
    // set_close_profit
    virtual void set_close_profit(const double& close_profit) {}
    // set the fee cost by trade
    virtual void set_fee(const double& fee) { hold_offset_ -= fee; account_->Fee += fee;}
    // 设定交易所编号
    virtual void set_exchange_id(const string& exchangeid) override { assign(account_->ExchangeID, exchangeid.c_str()); }
    // retrive the account field
    virtual boost::shared_ptr<CUTRtnAccountField> account_field();

public:
    // all relation positons trade by this currency
    // @key: the InstrumentID + Direction
    // @value: the PositionObject
    std::unordered_map<string, IPositionCellPtr> positions_;
    // wrapped account information
    boost::shared_ptr<CUTRtnAccountField> account_;
};
DECLARE_PTR(ICurrencyCell);

PANDORA_NAMESPACE_END