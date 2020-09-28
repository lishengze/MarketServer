#include "position_settle.h"
#include "quark/cxx/customDataType.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "quark/cxx/ut/UtData.h"
#include "../util/json.hpp"
#include "../util/time_util.h"
#include "../util/float_util.h"
#include "assign.h"
#include "../messager/ut_log.h"
#include "../messager/ding_talk.h"
#include "../messager/redis_publisher.h"

USING_PANDORA_NAMESPACE
using namespace std;
using json = nlohmann::json;

PositionSettle::PositionSettle(const std::string& account_name, const CAccountTypeType& account_type, const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid, const CPosiDirectionType& direction=PD_Net) : IPositionCell{account_name, account_type, instrumentid, asset_type, exchangeid, direction}
{

}

bool PositionSettle::frozen(const double& pos_quota, const double& currency_quota)
{
    if (!quote_checker(pos_quota, "frozen"))  return false;
    // 持仓配额更新
    if (pos_quota>0)
    {
        position_->FrozenBuy += pos_quota;
    }
    else
    {
        position_->FrozenSell -= pos_quota;
    }
    // 对于买卖而言都需要冻结 margin, 而这里的 margin 值应当始终为正
    position_->OrderMargin += currency_quota;
    
    PUBLISH_POSITION(position_field());
    return true;
}

void PositionSettle::unfrozen(const double& quota, const double& currency_quota)
{
    if (!quote_checker(quota, "unfrozen"))  return;
    if (quota>0)
    {
        if(!great_checker(position_->FrozenBuy, quota, "frozen_increase", "unfrozen_increase")) return;
        substract(position_->FrozenBuy, quota);
    }   
    else
    {
        if (!great_checker(position_->FrozenSell, quota, "frozen_decrease", "unfrozen_decrease"))    return;
        substract(position_->FrozenSell, -quota);
    }
    if (!great_checker(position_->OrderMargin, currency_quota, "frozen_margin", "abs_currency_quota")) return;
    substract(position_->OrderMargin, currency_quota);

    PUBLISH_POSITION(position_field());
}

void PositionSettle::confirm(const double& trade_pos_offset, const double& unfrozen_pos_quota, const double& trade_currency_offset, const double& unfrozen_currency_quota)
{
    if (!quote_checker(trade_pos_offset, "confirm quota trade") || !quote_checker(unfrozen_pos_quota, "confirm quota unfrozen"))  return;
    if (trade_pos_offset>0)
    {
        if (!great_checker(position_->FrozenBuy, unfrozen_pos_quota, "frozen_increase", "confirm_unfrozen_increase"))   return;
        substract(position_->FrozenBuy, unfrozen_pos_quota);    // 对冻结持仓进行解冻
    }
    else
    {
        if (!great_checker(position_->FrozenSell, unfrozen_pos_quota, "position_->FrozenSell", "confirm_unfrozen_decrease"))   return;
        substract(position_->FrozenSell, unfrozen_pos_quota);
    }
    hold_offset_ += trade_pos_offset;                    // 对持仓进行简单叠加，得到净持仓
    if (!great_checker(position_->OrderMargin, unfrozen_currency_quota, "frozen_margin", "abs_currency_quota")) return;
    substract(position_->OrderMargin, unfrozen_currency_quota);      // 减少报单冻结　margin
    position_->PositionMargin += trade_currency_offset;              // 增加持仓 margin

    PUBLISH_POSITION(position_field());
}

// void PositionSettle::init_position(const CUTRtnPositionField& position)
// {
//     hold_init_ = position.Position;
//     position_->Price = position.Price;
//     position_->FrozenBuy = position.FrozenBuy;
//     position_->FrozenSell = position.FrozenSell;

//     position_->PositionMargin = position.OrderMargin;
//     position_->OrderMargin = position.PosiDirection;
// }

std::string PositionSettle::position_str(long long& sequence_no)
{
    json position;
    // assign(position["SequenceNo"], sequence_no);
    // assign(position["AccountName"], account_name_);
    // assign(position["AccountType"], account_type_);
    // assign(position["ExchangeID"], exchange_id_);
    // assign(position["InstrumentID"], symbol_name());

    // assign(position["PosiDirection"], position_direction_);
    // assign(position["Position"], hold_init_+hold_offset_);
    // assign(position["OrderMargin"], position_->OrderMargin);
    // assign(position["PositionMargin"], position_->PositionMargin); 
    
    // assign(position["FrozenBuy"], frozen_increase());
    // assign(position["FrozenSell"], frozen_decrease());
    // assign(position["UpdateTime"], NanoTimeStr());
    
    return position.dump();
}

double PositionSettle::float_profit()
{ 
    double posi{position()}; 
    if (classic_settle_)
        return posi*(last_price_-position_->Price);
    else
        return posi*(1.0/position_->Price-1.0/last_price_);
}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////