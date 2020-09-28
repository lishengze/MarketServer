#include "digital_position_handler.h"
#include "currency_balance.h"
#include "boost/algorithm/string.hpp"
#include "position_settle.h"
#include "../util/float_util.h"
#include "../messager/ding_talk.h"
#include "quark/cxx/error/error_define.h"
#include "quark/cxx/customDataType.h"
#include "quark/cxx/Utils.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "../messager/ut_log.h"

USING_PANDORA_NAMESPACE
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// digital account riskcontrol baseclass, to solve some common issue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// digital account riskcontrol baseclass, to solve some common issue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DigitalSpotAccount::DigitalSpotAccount(const string& account_name, const char& account_type, IAssetFactory* asset_factory) : IPositionModule{account_name, account_type, AST_DigitalSpot, asset_factory}
{}

bool DigitalSpotAccount::req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField)
{
    LOG_TRACE("[SpotReqOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pReqCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pReqCreateOrderField->Direction) + "] [Volume=" + to_string(pReqCreateOrderField->Volume) + "] [Price=" + to_string(pReqCreateOrderField->Price) + "] [OrderLocalID=" + pReqCreateOrderField->OrderLocalID + "]");
    // check direction
    int direction = pReqCreateOrderField->Direction==BS_Long ? 1:-1;
    CurrencyPairType currency_pair = get_symbol_balance(pReqCreateOrderField->InstrumentID, pReqCreateOrderField->ExchangeID);

    if (!currency_pair.first->frozen(pReqCreateOrderField->Volume*direction))
        return false;
    if (!currency_pair.second->frozen(pReqCreateOrderField->Price*pReqCreateOrderField->Volume*direction*-1))
    {
        currency_pair.first->unfrozen(pReqCreateOrderField->Volume*direction);
        return false;
    }
    asset_factory_->currency_changed_notify(currency_pair.first);
    asset_factory_->currency_changed_notify(currency_pair.second);
    return true;
}

bool DigitalSpotAccount::rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField)
{
    LOG_TRACE("[SpotRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pReqCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pReqCreateOrderField->Direction) + "] [Volume=" + to_string(pReqCreateOrderField->Volume) + "] [Price=" + to_string(pReqCreateOrderField->Price) + "] [OrderLocalID=" + pReqCreateOrderField->OrderLocalID + "]");
    return rollback_order(pReqCreateOrderField, pReqCreateOrderField->Volume);
}

bool DigitalSpotAccount::on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo)
{
    if (pRspCreateOrderField->OrderStatus!=OST_Rejected)    return true;
    // if (pRspInfo->ErrorID==0)   return true;
    LOG_TRACE("[SpotRspInsertRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRspCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pRspCreateOrderField->Direction) + "] [Volume=" + to_string(pRspCreateOrderField->Volume) + "] [Price=" + to_string(pRspCreateOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRspCreateOrderField->OrderStatus) + "] [OrderLocalID=" + pRspCreateOrderField->OrderLocalID + "]");
    return rollback_order(pRspCreateOrderField, pRspCreateOrderField->Volume);
}

bool DigitalSpotAccount::on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo)
{
    // we can't finish order here
    if (pRspCancelOrderField->OrderStatus!=OST_Rejected && pRspCancelOrderField->OrderStatus!=OST_Killed && pRspCancelOrderField->OrderStatus!=OST_Fault)
        return true;
    LOG_TRACE("[SpotRspCancelRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRspCancelOrderField->InstrumentID + "[Direction=" + getDirectionString(pRspCancelOrderField->Direction) + "] [Volume=" + to_string(pRspCancelOrderField->Volume) + "] [Price=" + to_string(pRspCancelOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRspCancelOrderField->OrderStatus) + "] [OrderLocalID=" + pRspCancelOrderField->OrderLocalID + "]");
    // if (pRspInfo->ErrorID==0)   return true;    
    return rollback_order(pRspCancelOrderField, pRspCancelOrderField->Volume-pRspCancelOrderField->TradeVolume);
}

bool DigitalSpotAccount::on_rtn_order(const CUTRtnOrderField* pRtnOrderField)
{
    if (pRtnOrderField->OrderStatus!=OST_Rejected && pRtnOrderField->OrderStatus!=OST_Killed && pRtnOrderField->OrderStatus!=OST_Fault)
        return true;
    LOG_TRACE("[SpotRtnRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRtnOrderField->InstrumentID + "[Direction=" + getDirectionString(pRtnOrderField->Direction) + "] [Volume=" + to_string(pRtnOrderField->Volume) + "] [Price=" + to_string(pRtnOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRtnOrderField->OrderStatus) + "] [OrderLocalID=" + pRtnOrderField->OrderLocalID + "]");
    return rollback_order(pRtnOrderField, pRtnOrderField->Volume-pRtnOrderField->TradeVolume);
}

bool DigitalSpotAccount::on_rtn_trade(const CUTRtnTradeField* pRtnTrade)
{
    LOG_TRACE("[SpotRtnTrade] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRtnTrade->InstrumentID + "[Direction=" + getDirectionString(pRtnTrade->Direction) + "] [TradeVolume=" + to_string(pRtnTrade->MatchVolume) + "] [TradePrice=" + to_string(pRtnTrade->MatchPrice) + "] [Price=" + to_string(pRtnTrade->Price) + "] [OrderLocalID=" + pRtnTrade->OrderLocalID + "]");

    int direction = pRtnTrade->Direction==BS_Long ? 1:-1;

    CurrencyPairType currency_pair = get_symbol_balance(pRtnTrade->InstrumentID, pRtnTrade->ExchangeID);
    currency_pair.first->confirm(pRtnTrade->MatchVolume*direction, pRtnTrade->MatchVolume*direction);
    currency_pair.second->confirm(pRtnTrade->MatchPrice*pRtnTrade->MatchVolume*direction*-1, pRtnTrade->Price*pRtnTrade->MatchVolume*direction*-1);

    asset_factory_->currency_changed_notify(currency_pair.first);
    asset_factory_->currency_changed_notify(currency_pair.second);

    // calculate the fee
    if(pRtnTrade->FeeCurrency == currency_pair.first->currency_name())
    {
        currency_pair.first->set_fee(pRtnTrade->Fee);
    }
    else if(pRtnTrade->FeeCurrency == currency_pair.second->currency_name())
    {
        currency_pair.second->set_fee(pRtnTrade->Fee);
    }
    else
    {
        LOG_ERROR("Failed To Find Fee Currency " << pRtnTrade->FeeCurrency << " Of Symbol " << pRtnTrade->InstrumentID);
    }
//    InstrumentPropertyPtr inst_property = asset_factory_->retrieve_instrument_property(pRtnTrade->ExchangeID, pRtnTrade->InstrumentID);
//    if (!inst_property) return true;
//    if (currency_pair.first->currency_name()==inst_property->BaseCurrency)
//    {
//        currency_pair.first->set_fee(get_fee(inst_property, pRtnTrade, pRtnTrade->MatchPrice));
//    }
//    else if (currency_pair.second->currency_name()==inst_property->BaseCurrency)
//    {
//        currency_pair.second->set_fee(get_fee(inst_property, pRtnTrade, pRtnTrade->MatchPrice));
//    }
//    else
//    {
//        LOG_ERROR("Failed To Check The Currency " << inst_property->BaseCurrency << " Of Symbol " << pRtnTrade->InstrumentID);
//    }

    return true;
}

double DigitalSpotAccount::get_fee(InstrumentPropertyPtr inst_property, const CUTRtnTradeField* pRtnTrade, const double& match_price)
{
    if (pRtnTrade->OrderMaker==OM_Maker)
    {
        return inst_property->MakerFee*match_price*pRtnTrade->MatchVolume;
    }
    else
    {
        return inst_property->TakerFee*match_price*pRtnTrade->MatchVolume;
    }
}

DigitalSpotAccount::CurrencyPairType DigitalSpotAccount::get_symbol_balance(const std::string& instrumentid, const std::string& exchangeid)
{
    // get the symbol balance cached, if not exist then create one.
    // std::cout << symbol << std::endl;
    auto iterFindSymbol = SymbolBalance.find(instrumentid);
    if (iterFindSymbol!=SymbolBalance.end())
    {
        return iterFindSymbol->second;
    }
    else
    {
        vector<string> currencys;
        boost::algorithm::split(currencys, instrumentid, boost::is_any_of("_"));
        assert(currencys.size()==2);
        ICurrencyCellPtr currency_balance_left = asset_factory_->retrieve_currency(currencys[0], asset_type_, exchangeid);
        ICurrencyCellPtr currency_balance_right = asset_factory_->retrieve_currency(currencys[1], asset_type_, exchangeid);
        SymbolBalance.emplace(instrumentid, std::make_pair(currency_balance_left, currency_balance_right));
        return std::make_pair(currency_balance_left, currency_balance_right);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position future account riskcontrol, to solve the future commodity's riskcontrol
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DigitalFutureAccount::DigitalFutureAccount(const string& account_name, const char& account_type, IAssetFactory* asset_factory) : IPositionModule{account_name, account_type, AST_DigitalFuture, asset_factory}
{
}

bool DigitalFutureAccount::req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField)
{
    LOG_TRACE("[FutureReqOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pReqCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pReqCreateOrderField->Direction) + "] [Volume=" + to_string(pReqCreateOrderField->Volume) + "] [Price=" + to_string(pReqCreateOrderField->Price) + "] [OrderLocalID=" + pReqCreateOrderField->OrderLocalID + "]");

    // retrieve the inst property
    InstrumentPropertyPtr inst_property = asset_factory_->retrieve_instrument_property(pReqCreateOrderField->ExchangeID, pReqCreateOrderField->InstrumentID);
    if (!inst_property) return false;
    double margin_rate{inst_property->MarginRate}; 

    // check direction
    int direction = pReqCreateOrderField->Direction==BS_Long ? 1:-1;

    IPositionCellPtr position = asset_factory_->retrieve_position(inst_property->BaseCurrency, pReqCreateOrderField->InstrumentID, asset_type_, pReqCreateOrderField->ExchangeID,  pReqCreateOrderField->Direction, inst_property->ClassicSettle);
    ICurrencyCellPtr currency_balance = asset_factory_->retrieve_currency(inst_property->BaseCurrency, asset_type_, pReqCreateOrderField->ExchangeID);
    assert(position); assert(currency_balance);
    // calculate the quota
    double position_quota{pReqCreateOrderField->Volume*direction};
    double margin_quota{0.0};       // none direction relationship
    double order_price{convert_price(inst_property, pReqCreateOrderField->Price)};
    if (margin_rate < 1)
        margin_quota = order_price*pReqCreateOrderField->Volume*margin_rate*inst_property->Multiple;
    else
        margin_quota = pReqCreateOrderField->Volume*margin_rate;
    // operate the postion and currency
    if(!position->frozen(position_quota, margin_quota))
        return false;
    if (!currency_balance->frozen_margin(margin_quota))
    {
        position->unfrozen(position_quota, margin_quota);
        return false;
    }
    // store the recent update
    asset_factory_->position_changed_notify(position);
    asset_factory_->currency_changed_notify(currency_balance);

    return true;
}

bool DigitalFutureAccount::rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField)
{
    LOG_TRACE("[FutureRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pReqCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pReqCreateOrderField->Direction) + "] [Volume=" + to_string(pReqCreateOrderField->Volume) + "] [Price=" + to_string(pReqCreateOrderField->Price) + "] [OrderLocalID=" + pReqCreateOrderField->OrderLocalID + "]");
    return rollback_order(pReqCreateOrderField, pReqCreateOrderField->Volume);
}

bool DigitalFutureAccount::on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo)
{
    if (pRspCreateOrderField->OrderStatus!=OST_Rejected)    return true;
    LOG_TRACE("[FutureRspInsertRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRspCreateOrderField->InstrumentID + "[Direction=" + getDirectionString(pRspCreateOrderField->Direction) + "] [Volume=" + to_string(pRspCreateOrderField->Volume) + "] [Price=" + to_string(pRspCreateOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRspCreateOrderField->OrderStatus) + "] [OrderLocalID=" + pRspCreateOrderField->OrderLocalID + "]");
    // if (pRspInfo->ErrorID==0)   return true;   
    return rollback_order(pRspCreateOrderField, pRspCreateOrderField->Volume);
}

bool DigitalFutureAccount::on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo)
{
    // we can't finish order here
    if (pRspCancelOrderField->OrderStatus!=OST_Rejected && pRspCancelOrderField->OrderStatus!=OST_Killed && pRspCancelOrderField->OrderStatus!=OST_Fault)
        return true;
    LOG_TRACE("[FutureRspCancelRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRspCancelOrderField->InstrumentID + "[Direction=" + getDirectionString(pRspCancelOrderField->Direction) + "] [Volume=" + to_string(pRspCancelOrderField->Volume) + "] [Price=" + to_string(pRspCancelOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRspCancelOrderField->OrderStatus) + "] [OrderLocalID=" + pRspCancelOrderField->OrderLocalID + "]");
    // if (pRspInfo->ErrorID==0)   return true;   
    return rollback_order(pRspCancelOrderField, pRspCancelOrderField->Volume-pRspCancelOrderField->TradeVolume);
}

bool DigitalFutureAccount::on_rtn_order(const CUTRtnOrderField* pRtnOrderField)
{
    if (pRtnOrderField->OrderStatus!=OST_Rejected && pRtnOrderField->OrderStatus!=OST_Killed && pRtnOrderField->OrderStatus!=OST_Fault)
        return true;
    LOG_TRACE("[FutureRtnRollbackOrder] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRtnOrderField->InstrumentID + "[Direction=" + getDirectionString(pRtnOrderField->Direction) + "] [Volume=" + to_string(pRtnOrderField->Volume) + "] [Price=" + to_string(pRtnOrderField->Price) + "] [OrderStatus=" + getOrderStatusString(pRtnOrderField->OrderStatus) + "] [OrderLocalID=" + pRtnOrderField->OrderLocalID + "]");
    return rollback_order(pRtnOrderField, pRtnOrderField->Volume-pRtnOrderField->TradeVolume);
}

bool DigitalFutureAccount::on_rtn_trade(const CUTRtnTradeField* pRtnTrade)
{
    LOG_TRACE("[FutureRtnTrade] [AccountName=" + account_name_ + "] [AssetType=" + std::to_string(asset_type_) + "] [InstrumentID=" + pRtnTrade->InstrumentID + "[Direction=" + getDirectionString(pRtnTrade->Direction) + "] [TradeVolume=" + to_string(pRtnTrade->MatchVolume) + "] [TradePrice=" + to_string(pRtnTrade->MatchPrice) + "] [Price=" + to_string(pRtnTrade->Price) + "] [OrderLocalID=" + pRtnTrade->OrderLocalID + "]");
    InstrumentPropertyPtr inst_property = asset_factory_->retrieve_instrument_property(pRtnTrade->ExchangeID, pRtnTrade->InstrumentID);
    if (!inst_property) return false;

    int direction = pRtnTrade->Direction==BS_Long ? 1:-1;
    // retrieve the position object accord to the condition
    IPositionCellPtr position = asset_factory_->retrieve_position(inst_property->BaseCurrency, pRtnTrade->InstrumentID, asset_type_, pRtnTrade->ExchangeID, pRtnTrade->Direction, inst_property->ClassicSettle);
    ICurrencyCellPtr currency_balance = asset_factory_->retrieve_currency(inst_property->BaseCurrency, asset_type_, pRtnTrade->ExchangeID);
    assert(position); assert(currency_balance);
    // calculate the quota
    double position_offset{pRtnTrade->MatchVolume*direction};                   // 持仓调整偏移量，因为是净持仓，所以持仓调整为直接叠加
    double position_unfrozen{pRtnTrade->MatchVolume};                           // 解冻因下单造成的冻结量，与方向无关
    double match_price{convert_price(inst_property, pRtnTrade->MatchPrice)};    // 成交价转换
    double order_price{convert_price(inst_property, pRtnTrade->Price)};         // 报单价转换
    double margin_confirm{0.0};                                                 // 因为本次成交所形成的保证金，实际占用可能并不是这么多，净头寸问题
    double margin_unfrozen{0.0};                                                // 解冻保证金额
    set_margin(inst_property, pRtnTrade, match_price, order_price, margin_confirm, margin_unfrozen);
    
    LOG_TRACE("[AccountName=" + account_name_ + "] [position_offset=" + to_string(position_offset) + "] [position_unfrozen="+to_string(position_unfrozen) + "] [match_price=" + to_string(match_price)+"] [order_price=" + to_string(order_price) + "] [margin_confirm=" + to_string(margin_confirm) + "] [margin_unfrozen=" + to_string(margin_unfrozen) + "]");

    double margin_offset{0.0};                                                  // 保证金变化量调整
    double avg_price{position->avg_price()};                                    // 调整持仓均价
    double close_profit{0.0};                                                   // 平仓盈亏
    LOG_TRACE("[AccountName=" + account_name_ + "] [PreviousPosition] [position=" + to_string(position->position()) + "] [avg_price=" + to_string(position->avg_price()) + "] [position_margin=" + to_string(position->position_margin()) + "]");
    // 买入成交，形成多仓
    if (position_offset>0)
    {
        double net_position = position_offset + position->position();
        // 原始有空仓，对冲掉一部分，注意这里需要调整可用资金
        if (position->position()<0)
        {
            double close_volume{0.0};
            if (equal(net_position, 0))
            {
                margin_offset = -position->position_margin();
                close_volume = -position->position();
                // 不需要调整持仓均价，需要调整平仓盈亏
            }
            else if (net_position > 0)  // 原有的空仓少
            {
                // 更新后仓位为：margin_confirm/position_offset*net_position
                margin_offset = margin_confirm/position_offset*net_position - position->position_margin();
                avg_price = match_price;
                close_volume = -position->position();
            }
            else                        // 原有的空仓多
            {
                // 更新后仓位为：position->margin()/position->position()()*net_position
                margin_offset = position->position_margin()/position->position()*net_position - position->position_margin();
                close_volume = pRtnTrade->MatchVolume;
                // 不需要调整持仓均价
            } 
            if (inst_property->ClassicSettle)
                close_profit = close_volume*(position->avg_price()-match_price);
            else
                close_profit = close_volume*(match_price-position->avg_price());
            LOG_TRACE("[AccountName=" + account_name_ + "] [close_volume=" + to_string(close_volume) + "]");
        }   
        else    // 原始没有空仓，或者只有多仓
        {
            margin_offset = margin_confirm;
            avg_price = (position->position()*position->avg_price()+match_price*position_offset)/(position->position()+position_offset);
        }
    }
    else
    {
        double net_position = position_offset + position->position();
        // 原始存在多仓
        if (position->position()>0)
        {
            double close_volume{0.0};
            if (equal(net_position, 0))
            {
                margin_offset = -position->position_margin();
                close_volume = position->position();
            }
            else if (net_position > 0)  // 原始多仓比成交多
            {
                // 更新后仓位为：position->margin()/position->position()()*net_position
                margin_offset = position->position_margin()/position->position()*net_position - position->position_margin();       
                close_volume = pRtnTrade->MatchVolume;
            }
            else                        // 原始多仓比成交少
            {
                // 更新后仓位为：margin_confirm/position_offset*net_position
                margin_offset = margin_confirm/position_offset*net_position - position->position_margin();
                avg_price = match_price;
                close_volume = position->position();
            } 
            if (inst_property->ClassicSettle)
                close_profit = close_volume*(match_price - position->avg_price());
            else
                close_profit = close_volume*(position->avg_price() - match_price);
            LOG_TRACE("[AccountName=" + account_name_ + "] [close_volume=" + to_string(close_volume) + "]");  
        }
        else
        {
            margin_offset = margin_confirm;
            avg_price = (position->position()*position->avg_price()+match_price*position_offset)/(position->position()+position_offset);
        }
    }  
    double fee{get_fee(inst_property, pRtnTrade, match_price)};
    LOG_TRACE("[AccountName=" + account_name_ + "] [net_position=" + to_string(position_offset + position->position()) + "] [avg_price=" + to_string(avg_price) + "] [margin_offset=" + to_string(margin_offset) + "] [close_profit=" + to_string(close_profit) + "] [fee=" + to_string(fee) + "]");
    // adjust position
    position->set_avg_price(avg_price);
    position->confirm(position_offset, position_unfrozen, margin_offset, margin_unfrozen);
    // adjust currency balace
    currency_balance->set_close_profit(close_profit);
    currency_balance->set_fee(fee);
    currency_balance->confirm_margin(margin_offset, margin_unfrozen);
    // reset the recent update
    asset_factory_->currency_changed_notify(currency_balance);
    asset_factory_->position_changed_notify(position);

    return true;
}

double DigitalFutureAccount::convert_price(InstrumentPropertyPtr inst_property, const double& price)
{
    if (inst_property->ClassicSettle)
        return price;
    else
        return 1.0/price;
}

void DigitalFutureAccount::set_margin(InstrumentPropertyPtr inst_property, const CUTRtnTradeField* pRtnTrade, const double& match_price, const double& order_price, double& margin_confirm, double& margin_unfrozen)
{
    double margin_rate{inst_property->MarginRate};
    if (margin_rate < 1)
    {   
        margin_confirm = match_price*pRtnTrade->MatchVolume*margin_rate*inst_property->Multiple;
        margin_unfrozen = order_price*pRtnTrade->MatchVolume*margin_rate*inst_property->Multiple;
    }
    else
    {
        margin_confirm = pRtnTrade->MatchVolume*margin_rate;
        margin_unfrozen = pRtnTrade->MatchVolume*margin_rate;
    }
}

double DigitalFutureAccount::get_fee(InstrumentPropertyPtr inst_property, const CUTRtnTradeField* pRtnTrade, const double& match_price)
{
    if (pRtnTrade->OrderMaker==OM_Maker)
    {
        LOG_TRACE("[AccountName=" + account_name_ + "] [MakerFee=" + to_string(inst_property->MakerFee) + "] [match_price=" + to_string(match_price) + "] [Volume=" + to_string(pRtnTrade->MatchVolume) + "]");
        return inst_property->MakerFee*match_price*pRtnTrade->MatchVolume;
    }
    else
    {
        LOG_TRACE("[AccountName=" + account_name_ + "] [TakerFee=" + to_string(inst_property->TakerFee) + "] [match_price=" + to_string(match_price) + "] [Volume=" + to_string(pRtnTrade->MatchVolume) + "]");
        return inst_property->TakerFee*match_price*pRtnTrade->MatchVolume;
    }
}
