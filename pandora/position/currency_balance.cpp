#include "currency_balance.h"

#include "../redis/redis_api.h"
#include "../messager/ut_log.h"
#include "../messager/ding_talk.h"
#include "../messager/redis_publisher.h"
#include "../util/float_util.h"
#include "quark/cxx/ut/UtPackageDesc.h"

USING_PANDORA_NAMESPACE
using namespace std;

CurrencyBalance::CurrencyBalance(const string& account_name, const char& account_type, const string& currency_name, const char& asset_type) :  ICurrencyCell{account_name, account_type, currency_name, asset_type}
{}

bool CurrencyBalance::frozen(const double& quota)
{
    cout<<"CurrencyBalance::frozen"<<endl;
    if (!quote_checker(quota, "frozen"))  return false;
    if (quota>0)
    {
        account_->FrozenBuy += quota;
        // operate_direction_ = PD_Long;
        LOG_TRACE("[FrozenBuy] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");
    }
    else
    {
        account_->FrozenSell -= quota;
        // operate_direction_ = PD_Short;
        LOG_TRACE("[FrozenSell] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");

    }
    PUBLISH_ACCOUNT(account_field());
    
    return true;
}

bool CurrencyBalance::unfrozen(const double& quota)
{
    if (!quote_checker(quota, "unfrozen"))  return false;
    if (quota>0)
    {
        if(!great_checker(account_->FrozenBuy, quota, "frozen_increase", "unfrozen_increase")) return false;
        substract(account_->FrozenBuy, quota);
        // operate_direction_ = PD_Long;
        LOG_TRACE("[UnFrozenBuy] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");
    }   
    else
    {
        if (!great_checker(account_->FrozenSell, quota, "frozen_decrease", "unfrozen_decrease"))    return false;
        substract(account_->FrozenSell, -quota);
        // operate_direction_ = PD_Short;
        LOG_TRACE("[UnFrozenSell] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");
    }
    
    PUBLISH_ACCOUNT(account_field());
    return true;
}

bool CurrencyBalance::confirm(const double& quota_trade, const double& quota_unfrozen)
{
    if (!quote_checker(quota_trade, "confirm quota trade") || !quote_checker(quota_unfrozen, "confirm quota unfrozen"))  return false;
    if (quota_trade>0)
    {
        if (!great_checker(account_->FrozenBuy, quota_unfrozen, "frozen_increase", "confirm_unfrozen_increase"))   return false;
        hold_offset_ += quota_trade;
        substract(account_->FrozenBuy, quota_unfrozen);
        // operate_direction_ = PD_Long;
        LOG_TRACE("[ConfirmBuy] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [QuotaTrade=" + to_string(quota_trade) + "] [QuotaUnfrozen=" + to_string(quota_unfrozen) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");
    }
    else
    {
        if (!great_checker(account_->FrozenSell, quota_unfrozen, "account_->FrozenSell", "confirm_unfrozen_decrease"))   return false;
        hold_offset_ += quota_trade;
        substract(account_->FrozenSell, -quota_unfrozen);
        LOG_TRACE("[ConfirmSell] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [QuotaTrade=" + to_string(quota_trade) + "] [QuotaUnfrozen=" + to_string(quota_unfrozen) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");
        // operate_direction_ = PD_Short;
    }
    PUBLISH_ACCOUNT(account_field());
    return true;
}

bool CurrencyBalance::confirm_for_transact(const double& quota_trade, const double& quota_unfrozen)
{
    hold_offset_ += quota_trade;
    substract(account_->FrozenSell, quota_unfrozen);
    LOG_TRACE("[ConfirmBuy] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [QuotaTrade=" + to_string(quota_trade) + "] [QuotaUnfrozen=" + to_string(quota_unfrozen) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");

    PUBLISH_ACCOUNT(account_field());
    return true;
}

bool CurrencyBalance::frozen_margin(const double& quota)
{
    if (!quote_checker(quota, "frozen_margin"))  return false;

    account_->OrderMargin += quota; // 冻结报单保证金

    LOG_TRACE("[FrozenMargin] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");

    PUBLISH_ACCOUNT(account_field());
    
    return true;
}

void CurrencyBalance::unfrozen_margin(const double& quota)
{
    if (!quote_checker(quota, "unfrozen margin"))  return;

    if(!great_checker(account_->OrderMargin, quota, "frozen_increase", "unfrozen_increase")) return;
    substract(account_->OrderMargin, quota);    // 解冻报单保证金

    LOG_TRACE("[UnFrozenMargin] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [quota=" + to_string(quota) + "] [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");

    PUBLISH_ACCOUNT(account_field());
}

void CurrencyBalance::confirm_margin(const double& quota_trade, const double& quota_unfrozen)
{
    if (!quote_checker(quota_trade, "confirm quota trade") || !quote_checker(quota_unfrozen, "confirm quota unfrozen"))  return;
    if (!great_checker(account_->OrderMargin, quota_unfrozen, "frozen_increase", "confirm_unfrozen_increase"))   return;
    account_->PositionMargin += quota_trade;                                    // 调整　position_margin 
    assert(account_->PositionMargin>0 || equal(account_->PositionMargin, 0));   // 校验 position_margin
    substract(account_->OrderMargin, quota_unfrozen);                           // 调整报单保证金

    LOG_TRACE("[ConfirmMargin] [AccountName=" + account_name() + "] [AccountType=" + to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + to_string(asset_type()) + "] [QuotaTrade=" + to_string(quota_trade) + "] [QuotaUnfrozen=" + to_string(quota_unfrozen) + " [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [FrozenIncrease=" + to_string(account_->FrozenBuy) + "] [FrozenDecrease=" + to_string(account_->FrozenSell) + "]");

    PUBLISH_ACCOUNT(account_field());
}

// void CurrencyBalance::init_currency(const CUTRtnAccountField& account)
// {
//     // 初始持仓量
//     hold_init_ = account.CurrencyQuantity;
//     // 报单保证金占有量
//     account_->OrderMargin = account.OrderMargin;
//     // 持仓保证金占有量
//     account_->PositionMargin = account.PositionMargin;
// }

//string CurrencyBalance::currency_str(long long& sequence_no)
//{
//    nlohmann::json account;
//     assign(account["SequenceNo"], sequence_no);
//     assign(account["AccountName"], account_name_);
//     assign(account["AccountType"], account_type_);
//     assign(account["CurrencyName"], asset_name_);
//     assign(account["ExchangeID"], exchange_id_);
//
//     assign(account["Available"], hold_init_+hold_offset_);
//     assign(account["OrderMargin"], 0);
//     assign(account["PositionMargin"], 0);
//
//     // assign(account["Offset"], hold_offset_);
//     assign(account["FrozenBuy"], frozen_increase());
//     assign(account["FrozenSell"], frozen_decrease());
//     assign(account["UpdateTime"], NanoTimeStr());
//
//    return account.dump();
//}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// /// Margin currency balance    
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MarginCurrencyBalance::MarginCurrencyBalance(const string& account_name, const char& account_type, const string& currency_name, const char& asset_type) :  ICurrencyCell{account_name, account_type, currency_name, asset_type}
// {}

// bool MarginCurrencyBalance::frozen(const double& quota)
// {
//     if (!quote_checker(quota, "frozen_margin"))  return false;

//     if (risk_control_ && !risk_control_->frozen_margin(hold_init_, hold_offset_, quota, position_margin_, order_margin_))
//     {   
//         string tips = std::to_string(ERROR_FROZEN_BUY) + " [AccountName=" + account_->AccountName + "] [Currency=" + account_->CurrencyName + "] [quota=" + to_string(quota) + "] riskctrl Failed [Init=" + to_string(hold_init_) + "] [Offset=" + to_string(hold_offset_) + "] [PositionMargin=" + to_string(position_margin_) + "] [OrderMargin=" + to_string(order_margin_) + "] [Threshold=" + to_string(risk_control_->Threshold) + "]";
//         LOG_WARN(tips);
//         DINGTALK(account_->AccountName, "Warning", ERROR_FROZEN_BUY, tips);
//         return false;
//     }
//     order_margin_ += quota; // 冻结报单保证金

//     PUBLISH_ACCOUNT(account_field());
    
//     return true;
// }

// void MarginCurrencyBalance::unfrozen(const double& quota)
// {
//     if (!quote_checker(quota, "unfrozen margin"))  return;

//     if(!great_checker(order_margin_, quota, "frozen_increase", "unfrozen_increase")) return;
//     substract(order_margin_, quota);    // 解冻报单保证金

//     PUBLISH_ACCOUNT(account_field());
// }

// void MarginCurrencyBalance::confirm(const double& quota_trade, const double& quota_unfrozen)
// {
//     if (!quote_checker(quota_trade, "confirm quota trade") || !quote_checker(quota_unfrozen, "confirm quota unfrozen"))  return;
//     if (!great_checker(order_margin_, quota_unfrozen, "frozen_increase", "confirm_unfrozen_increase"))   return;
//     position_margin_ += quota_trade;                            // 调整　position_margin 
//     assert(position_margin_>0 || equal(position_margin_, 0));   
//     substract(order_margin_, quota_unfrozen);                   // 调整报单保证金

//     PUBLISH_ACCOUNT(account_field());
// }

// void MarginCurrencyBalance::init_currency(const CUTRtnAccountField& account)
// {
//     // 初始总量
//     hold_init_ = account.CurrencyQuantity;
//     // 报单保证金占有量
//     order_margin_ = account.OrderMargin;
//     // 持仓保证金占有量
//     position_margin_ = account.PositionMargin;
// }

// string MarginCurrencyBalance::currency_str(long long& sequence_no)
// {
//     nlohmann::json account; 
//     // assign(account["SequenceNo"], sequence_no);
//     // assign(account["AccountName"], account_name_);
//     // assign(account["AccountType"], account_type_);
//     // assign(account["CurrencyName"], asset_name_);
//     // assign(account["ExchangeID"], exchange_id_);
    
//     // assign(account["Available"], hold_init_+hold_offset_);
//     // assign(account["OrderMargin"], order_margin_);
//     // assign(account["PositionMargin"], position_margin_); 
    
//     // assign(account["FrozenBuy"], frozen_increase());
//     // assign(account["FrozenSell"], frozen_decrease());
//     // assign(account["UpdateTime"], NanoTimeStr());
    
//     return account.dump();
// }

// boost::shared_ptr<CUTRtnAccountField> MarginCurrencyBalance::account_field()
// {
//     boost::shared_ptr<CUTRtnAccountField> pAccount{new CUTRtnAccountField()};
//     memset(pAccount.get(), 0, sizeof(CUTRtnAccountField));
//     // assign(pAccount->AccountName, account_name_.c_str());
//     // assign(pAccount->AccountType, account_type_);
//     // assign(pAccount->CurrencyName, asset_name_.c_str());
//     // assign(pAccount->ExchangeID, exchange_id_.c_str());
    
//     // assign(pAccount->CurrencyQuantity, hold());
//     // assign(pAccount->Available, available());
//     // assign(pAccount->OrderMargin, order_margin_);
//     // assign(pAccount->PositionMargin, position_margin_);
    
//     // assign(pAccount->FrozenBuy, frozen_increase());
//     // assign(pAccount->FrozenSell, frozen_decrease());
//     // assign(pAccount->UpdateTime, NanoTimeStr().c_str());
//     // assign(pAccount->CurrencyID, currency_id_);
//     // assign(pAccount->AssetType, asset_type_);

//     return pAccount;
// }

// void MarginCurrencyBalance::bind_riskcontrol(CurrencyRiskControlPtr riskctrl)
// {
//     assert(riskctrl);
//     if (!risk_control_)
//         risk_control_ = riskctrl;
//     else
//         risk_control_->set_config(riskctrl);
//     // set the init hold value
//     hold_init_ = risk_control_->Hold;
//     LOG_DEBUG("[MarginCurrencyBalance] Set Riskconfig [AccountName=" << account_->AccountName << "] [Currency=" << account_->CurrencyName << "] [hold=" << risk_control_->Hold << "] [Threshold=" << risk_control_->Threshold << "]");
//     PUBLISH_ACCOUNT(account_field());
// }