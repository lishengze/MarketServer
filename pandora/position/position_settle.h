#pragma once
#include "../pandora_declare.h"
#include "asset_cell.h"

PANDORA_NAMESPACE_START

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the position control, 风控绑定到基本持仓单位，比如说合约的多持仓，或者合约的空持仓，或者合约的净持仓
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PositionSettle : public IPositionCell
{
public:
    PositionSettle(const std::string& account_name, const CAccountTypeType& account_type, const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid, const CPosiDirectionType& direction);
    virtual ~PositionSettle(){}

    // available position, exclude frozen
    virtual double available() override { return hold_init_+hold_offset_-position_->FrozenSell; }
    // total position include 
    virtual double hold() override { return hold_init_+hold_offset_; }

    // execute the event
    virtual bool frozen(const double& quota, const double& currency_quota) override;
    virtual void unfrozen(const double& quota, const double& currency_quota) override;
    virtual void confirm(const double& trade_quota, const double& unfrozen_quota, const double& trade_currency_quota, const double& unfrozen_currency_quota) override;
    // position occupy margin
    virtual double position_margin() { return position_->OrderMargin; }
    // oder occupy margin
    virtual double order_margin() { return position_->PositionMargin; }
    // float profit
    virtual double float_profit() override;
private:
    // position string 
    std::string position_str(long long& sequence_no);
    // record trade & margin
    std::vector<std::pair<double, double>> trade_volume;
};
DECLARE_PTR(PositionSettle);













//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FuturePositionSettle : public IPositionCell
{
public:
    FuturePositionSettle(const std::string& account_name, const char& account_type, const string& exchangeid, const string& symbol, char direction);
    virtual ~FuturePositionSettle(){}

    // frozen value
    virtual double available() override { return position_offset_yesterday_+position_offset_today_; }
    virtual double hold() override { return 0; }
    virtual double position_today() { return position_offset_today_; }
    virtual double position_yesterday() { return position_offset_yesterday_; }
    // virtual void bind_riskcontrol(PositionRiskControlPtr riskctrl) {}

    virtual bool frozen(const double& quota, const double& currency_quota) override { return true; }
    virtual void unfrozen(const double& quota, const double& currency_quota) override {}
    virtual void confirm(const double& confirm_quota, const double& unfrozen_quota, const double& confirm_currency_quota, const double& unfrozen_currency_quota) override {}

    // account change offset
    double position_offset_today_{0.0};
    // account change offset yesterday
    double position_offset_yesterday_{0.0};
private:
    // position string 
    std::string position_str(long long& sequence_no);
    // risk control
    // PositionRiskControlPtr risk_control_{nullptr};
};
DECLARE_PTR(FuturePositionSettle);

// 国内期货以合约为单位进行风控，分两个方向
class DemesticFuturePositionSettle : public IPositionCell
{

};
DECLARE_PTR(DemesticFuturePositionSettle);


PANDORA_NAMESPACE_END