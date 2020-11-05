#include "asset_cell.h"

#include "../messager/ut_log.h"
#include "../messager/ding_talk.h"
#include "../messager/redis_publisher.h"
#include "../util/float_util.h"
#include "quark/cxx/error/error_define.h"
#include "quark/cxx/customDataType.h"
#include "quark/cxx/ut/UtData.h"
#include "quark/cxx/ut/UtCopyEntity.h"

USING_PANDORA_NAMESPACE

bool IAssetCell::quote_checker(double quota, const string& tips)
{
    if (equal(fabs(quota), 0))
    {
        string assemble_tips{"[" + tips + " = " + std::to_string(quota) + "]"};
        LOG_ERROR("[AccountName = " + account_name() + "] " +assemble_tips);
        DINGTALK(account_name(), "Error", ERROR_FROZEN_QUOTA_CHECKER, assemble_tips+" equal 0.");
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position cell and currency cell base general interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IAssetCell::IAssetCell()
{}

bool IAssetCell::great_checker(double subtrahend, double minuend, const string& tips_1, const string& tips_2)
{
    if (less(subtrahend, fabs(minuend)))
    {
        std::string assemble_tips{"[" + tips_1 + " = " + std::to_string(subtrahend) + "] is less than [" + tips_2 + " = " + std::to_string(minuend) + "]"};
        LOG_ERROR("[AccountName = " + account_name() + "] " + assemble_tips);
        DINGTALK(account_name(), "Error", ERROR_FROZEN_GREAT_CHECKER, assemble_tips);
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position cell here general interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPositionCell::IPositionCell(const std::string& account_name, const CAccountTypeType& account_type, const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid, const CPosiDirectionType& direction) : position_{new CUTRtnPositionField{}}
{
    assign(position_->AccountName, account_name.c_str());
    assign(position_->AccountType, account_type);
    assign(position_->InstrumentID, instrumentid.c_str());
    assign(position_->AssetType, asset_type);
    assign(position_->ExchangeID, exchangeid.c_str());
    assign(position_->PosiDirection, direction);
    assign(position_->PositionID, NanoTime());
}

void IPositionCell::init_position(const CUTRtnPositionField& position)
{
    // UTCopyRtnPositionEntity(position_.get(), &position);
    hold_init_ = position.Position;
    position_->Price = position.Price;
    position_->FrozenBuy = position.FrozenBuy;
    position_->FrozenSell = position.FrozenSell;

    position_->PositionMargin = position.OrderMargin;
    position_->OrderMargin = position.PosiDirection;

    LOG_TRACE("[InitPosition] [AccountName=" + account_name() + "] [AccountType=" + std::to_string(account_type()) + "] [InstrumentID=" + position_->InstrumentID + "] [AssetType=" + std::to_string(asset_type()) + "] [ExchangeID=" + position_->ExchangeID + "] [Direction=" + position_->PosiDirection + "] [Init=" + std::to_string(hold_init_) + "] [Price=" + std::to_string(position_->Price) + "] [FrozenBuy=" + std::to_string(position_->FrozenBuy) + "] [FrozenSell=" + std::to_string(position_->FrozenSell) + "] [PositionMargin=" + std::to_string(position_->PositionMargin) + "] [OrderMargin=" + std::to_string(position_->OrderMargin) + "]");
}

boost::shared_ptr<CUTRtnPositionField> IPositionCell::position_field()
{
    assign(position_->UpdateTime, utrade::pandora::NanoTimeStr().c_str());
    assign(position_->Position, position());
    return position_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// currency in account general interface
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ICurrencyCell::ICurrencyCell(const string& account_name, const CAccountTypeType& account_type, const string& capital_name, const CAssetTypeType& asset_type) : account_{new CUTRtnAccountField{}}
{
    assign(account_->AccountName, account_name.c_str());
    assign(account_->AccountType, account_type);
    assign(account_->CurrencyName, capital_name.c_str());
    assign(account_->AssetType, asset_type);
    assign(account_->CurrencyID, NanoTime());
}

// 因为绑定的 position 有可能是别的币种买的，不一定是当前币种．如果需要彻底解决这个问题，需要在当前币种中自己管理独有的币种对应持仓
void ICurrencyCell::bind_position(IPositionCellPtr position, const string& instrumentid, const CPosiDirectionType& direction/* =PD_Net */) 
{ 
    positions_.emplace(instrumentid+direction, position);
    // bind to the currency id
    position->bind_currency_id(account_->CurrencyID); 
}

IPositionCellPtr ICurrencyCell::retrieve_position(const string& instrumentid, const CPosiDirectionType& direction/* =PD_Net */)
{
    auto iter_find_position = positions_.find(instrumentid+direction);
    if (iter_find_position!=positions_.end())
    {
        return iter_find_position->second;
    }
    return nullptr;
}

void ICurrencyCell::init_currency(const CUTRtnAccountField& account)
{
    // UTCopyRtnAccountEntity(account_.get(), &account);
    // 初始持仓量
    hold_init_ = account.CurrencyQuantity;
    // 报单保证金占有量
    account_->OrderMargin = account.OrderMargin;
    // 持仓保证金占有量
    account_->PositionMargin = account.PositionMargin;

    account_->Frozen = account.OrderMargin;

    LOG_TRACE("[InitCurrency] [AccountName=" + account_name() + "] [AccountType=" + std::to_string(account_type()) + "] [CurrencyName=" + currency_name() + "] [AssetType=" + std::to_string(asset_type()) + "] [Init=" + std::to_string(hold_init_) + "] [PositionMargin=" + std::to_string(account_->PositionMargin) + "] [OrderMargin=" + std::to_string(account_->OrderMargin) + "]");
}

boost::shared_ptr<CUTRtnAccountField> ICurrencyCell::account_field()
{
    assign(account_->UpdateTime, utrade::pandora::NanoTimeStr().c_str());
    assign(account_->CurrencyQuantity, hold());
    return account_;
}