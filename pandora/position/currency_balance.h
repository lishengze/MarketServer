#pragma once
#include "../pandora_declare.h"
#include "asset_cell.h"

PANDORA_NAMESPACE_START

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// normal currency balance    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CurrencyBalance : public ICurrencyCell
{
public:
    CurrencyBalance(const std::string& account_name, const char& account_type, const string& currency_name, const char& asset_type);
    virtual ~CurrencyBalance() {}

    // solve frozen, unfrozen, confirm
    // invoke by create order
    virtual bool frozen(const double& quota) override;
    // invoke by riskctrl or cancel order
    virtual bool unfrozen(const double& quota) override;
    // invoke by trade
    virtual bool confirm(const double& quota_trade, const double& quota_unfrozen) override;

    virtual bool confirm_for_transact(const double& quota_trade, const double& quota_unfrozen) override;
        // invoke by create margin order
    virtual bool frozen_margin(const double& quota) override;
    // invoke by cancel margin order
    virtual void unfrozen_margin(const double& quota) override;
    // invoke by margin trade
    virtual void confirm_margin(const double& quota_trade, const double& quota_unfrozen) override;

    // the currency quantity avalilable
    virtual double available() override { return hold_init_+hold_offset_ - account_->PositionMargin - account_->OrderMargin - account_->FrozenSell; }
    // the total hold include frozen quantity
    virtual double hold() override { return hold_init_+hold_offset_; }
    
    // set btc value of current currency
    virtual void set_btc_per_currency(const double& value) { btc_per_currency=value; }
    // set close profit
    virtual void set_close_profit(const double& close_profit) override { hold_offset_+=close_profit; }
    // btc balance
    double btc_balance() { return hold()*btc_per_currency; }

private:
    // get currency str
    // string currency_str(long long& sequence_no);
    // btc_per_currency
    double btc_per_currency{0.0};
};
DECLARE_PTR(CurrencyBalance);

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// /// Margin currency balance    
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // 保证金全部放在 order_margin 与　position_margin 里面进行记录，而账户浮动 offset 由　平仓盈亏和手续费进行更新
// class MarginCurrencyBalance : public ICurrencyCell
// {
// public:
//     MarginCurrencyBalance(const std::string& account_name, const char& account_type, const string& currency_name, const char& asset_type);
//     virtual ~MarginCurrencyBalance() {}
//     // solve frozen, unfrozen, confirm
//     // invoke by create order
//     virtual bool frozen(const double& quota) override;
//     // invoke by riskctrl or cancel order
//     virtual void unfrozen(const double& quota) override;
//     // invoke by trade
//     virtual void confirm(const double& quota_trade, const double& quota_unfrozen) override;

//     // the currency quantity avalilable
//     virtual double available() override { return hold_init_+hold_offset_-position_margin_-order_margin_; }
//     // the total hold include frozen quantity
//     virtual double hold() override { return hold_init_+hold_offset_; }
//     // init the currency
//     virtual void init_currency(const CUTRtnAccountField& account) override;
//     // get currency str
//     string currency_str(long long& sequence_no);
//     // set close profit
//     virtual void set_close_profit(const double& close_profit) override { hold_offset_+=close_profit; }
//     //get the account information in field format
//     virtual boost::shared_ptr<CUTRtnAccountField> account_detail();
//     // bind currency riskctrl
//     virtual void bind_riskcontrol(CurrencyRiskControlPtr riskctrl);
// private:
//     // order frozen margin
//     double order_margin_{0.0};
//     // position occupy margin
//     double position_margin_{0.0};
//     // bind risk config module
//     CurrencyRiskControlPtr risk_control_;
// };
// DECLARE_PTR(MarginCurrencyBalance);

PANDORA_NAMESPACE_END