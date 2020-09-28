/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UTType.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file UtPackageDesc.cpp
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190716      创建该文件
/////////////////////////////////////////////////////////////////////////
#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include "quark/cxx/ut/UtData.h"
#include "../util/json.hpp"

PANDORA_NAMESPACE_START

inline std::string UTData2Jason(const void* data, short msg_type)
{
    nlohmann::json jsonFields;
    switch(msg_type)
    {
        case UT_FID_RspInfo:
        {
            auto pRspInfoField = (CUTRspInfoField*)data;
            assign(jsonFields["ErrorID"], pRspInfoField->ErrorID);
            assign(jsonFields["ErrorMsg"], pRspInfoField->ErrorMsg);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqCreateOrder:
        {
            auto pReqCreateOrderField = (CUTReqCreateOrderField*)data;
            assign(jsonFields["ExchangeID"], pReqCreateOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pReqCreateOrderField->InstrumentID);
            assign(jsonFields["Price"], pReqCreateOrderField->Price);
            assign(jsonFields["Volume"], pReqCreateOrderField->Volume);
            assign(jsonFields["Direction"], pReqCreateOrderField->Direction);
            assign(jsonFields["OffsetFlag"], pReqCreateOrderField->OffsetFlag);
            assign(jsonFields["OrderLocalID"], pReqCreateOrderField->OrderLocalID);
            assign(jsonFields["OrderMaker"], pReqCreateOrderField->OrderMaker);
            assign(jsonFields["OrderType"], pReqCreateOrderField->OrderType);
            assign(jsonFields["LandTime"], pReqCreateOrderField->LandTime);
            assign(jsonFields["SendTime"], pReqCreateOrderField->SendTime);
            assign(jsonFields["StrategyOrderID"], pReqCreateOrderField->StrategyOrderID);
            assign(jsonFields["OrderMode"], pReqCreateOrderField->OrderMode);
            assign(jsonFields["AssetType"], pReqCreateOrderField->AssetType);
            assign(jsonFields["TradeChannel"], pReqCreateOrderField->TradeChannel);
            assign(jsonFields["OrderXO"], pReqCreateOrderField->OrderXO);
            assign(jsonFields["PlatformTime"], pReqCreateOrderField->PlatformTime);
            assign(jsonFields["OrderSysID"], pReqCreateOrderField->OrderSysID);
            assign(jsonFields["OrderForeID"], pReqCreateOrderField->OrderForeID);
            assign(jsonFields["CreateTime"], pReqCreateOrderField->CreateTime);
            assign(jsonFields["ModifyTime"], pReqCreateOrderField->ModifyTime);
            assign(jsonFields["RspLocalTime"], pReqCreateOrderField->RspLocalTime);
            assign(jsonFields["Cost"], pReqCreateOrderField->Cost);
            assign(jsonFields["TradePrice"], pReqCreateOrderField->TradePrice);
            assign(jsonFields["TradeVolume"], pReqCreateOrderField->TradeVolume);
            assign(jsonFields["TradeValue"], pReqCreateOrderField->TradeValue);
            assign(jsonFields["OrderStatus"], pReqCreateOrderField->OrderStatus);
            assign(jsonFields["SessionID"], pReqCreateOrderField->SessionID);
            assign(jsonFields["RequestID"], pReqCreateOrderField->RequestID);
            assign(jsonFields["RequestForeID"], pReqCreateOrderField->RequestForeID);
            assign(jsonFields["Fee"], pReqCreateOrderField->Fee);
            assign(jsonFields["FeeCurrency"], pReqCreateOrderField->FeeCurrency);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspCreateOrder:
        {
            auto pRspCreateOrderField = (CUTRspCreateOrderField*)data;
            assign(jsonFields["ExchangeID"], pRspCreateOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRspCreateOrderField->InstrumentID);
            assign(jsonFields["Price"], pRspCreateOrderField->Price);
            assign(jsonFields["Volume"], pRspCreateOrderField->Volume);
            assign(jsonFields["Direction"], pRspCreateOrderField->Direction);
            assign(jsonFields["OffsetFlag"], pRspCreateOrderField->OffsetFlag);
            assign(jsonFields["OrderLocalID"], pRspCreateOrderField->OrderLocalID);
            assign(jsonFields["OrderMaker"], pRspCreateOrderField->OrderMaker);
            assign(jsonFields["OrderType"], pRspCreateOrderField->OrderType);
            assign(jsonFields["LandTime"], pRspCreateOrderField->LandTime);
            assign(jsonFields["SendTime"], pRspCreateOrderField->SendTime);
            assign(jsonFields["StrategyOrderID"], pRspCreateOrderField->StrategyOrderID);
            assign(jsonFields["OrderMode"], pRspCreateOrderField->OrderMode);
            assign(jsonFields["AssetType"], pRspCreateOrderField->AssetType);
            assign(jsonFields["TradeChannel"], pRspCreateOrderField->TradeChannel);
            assign(jsonFields["OrderXO"], pRspCreateOrderField->OrderXO);
            assign(jsonFields["PlatformTime"], pRspCreateOrderField->PlatformTime);
            assign(jsonFields["OrderSysID"], pRspCreateOrderField->OrderSysID);
            assign(jsonFields["OrderForeID"], pRspCreateOrderField->OrderForeID);
            assign(jsonFields["CreateTime"], pRspCreateOrderField->CreateTime);
            assign(jsonFields["ModifyTime"], pRspCreateOrderField->ModifyTime);
            assign(jsonFields["RspLocalTime"], pRspCreateOrderField->RspLocalTime);
            assign(jsonFields["Cost"], pRspCreateOrderField->Cost);
            assign(jsonFields["TradePrice"], pRspCreateOrderField->TradePrice);
            assign(jsonFields["TradeVolume"], pRspCreateOrderField->TradeVolume);
            assign(jsonFields["TradeValue"], pRspCreateOrderField->TradeValue);
            assign(jsonFields["OrderStatus"], pRspCreateOrderField->OrderStatus);
            assign(jsonFields["SessionID"], pRspCreateOrderField->SessionID);
            assign(jsonFields["RequestID"], pRspCreateOrderField->RequestID);
            assign(jsonFields["RequestForeID"], pRspCreateOrderField->RequestForeID);
            assign(jsonFields["Fee"], pRspCreateOrderField->Fee);
            assign(jsonFields["FeeCurrency"], pRspCreateOrderField->FeeCurrency);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnOrder:
        {
            auto pRtnOrderField = (CUTRtnOrderField*)data;
            assign(jsonFields["ExchangeID"], pRtnOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnOrderField->InstrumentID);
            assign(jsonFields["Price"], pRtnOrderField->Price);
            assign(jsonFields["Volume"], pRtnOrderField->Volume);
            assign(jsonFields["Direction"], pRtnOrderField->Direction);
            assign(jsonFields["OffsetFlag"], pRtnOrderField->OffsetFlag);
            assign(jsonFields["OrderLocalID"], pRtnOrderField->OrderLocalID);
            assign(jsonFields["OrderMaker"], pRtnOrderField->OrderMaker);
            assign(jsonFields["OrderType"], pRtnOrderField->OrderType);
            assign(jsonFields["LandTime"], pRtnOrderField->LandTime);
            assign(jsonFields["SendTime"], pRtnOrderField->SendTime);
            assign(jsonFields["StrategyOrderID"], pRtnOrderField->StrategyOrderID);
            assign(jsonFields["OrderMode"], pRtnOrderField->OrderMode);
            assign(jsonFields["AssetType"], pRtnOrderField->AssetType);
            assign(jsonFields["TradeChannel"], pRtnOrderField->TradeChannel);
            assign(jsonFields["OrderXO"], pRtnOrderField->OrderXO);
            assign(jsonFields["PlatformTime"], pRtnOrderField->PlatformTime);
            assign(jsonFields["OrderSysID"], pRtnOrderField->OrderSysID);
            assign(jsonFields["OrderForeID"], pRtnOrderField->OrderForeID);
            assign(jsonFields["CreateTime"], pRtnOrderField->CreateTime);
            assign(jsonFields["ModifyTime"], pRtnOrderField->ModifyTime);
            assign(jsonFields["RspLocalTime"], pRtnOrderField->RspLocalTime);
            assign(jsonFields["Cost"], pRtnOrderField->Cost);
            assign(jsonFields["TradePrice"], pRtnOrderField->TradePrice);
            assign(jsonFields["TradeVolume"], pRtnOrderField->TradeVolume);
            assign(jsonFields["TradeValue"], pRtnOrderField->TradeValue);
            assign(jsonFields["OrderStatus"], pRtnOrderField->OrderStatus);
            assign(jsonFields["SessionID"], pRtnOrderField->SessionID);
            assign(jsonFields["RequestID"], pRtnOrderField->RequestID);
            assign(jsonFields["RequestForeID"], pRtnOrderField->RequestForeID);
            assign(jsonFields["Fee"], pRtnOrderField->Fee);
            assign(jsonFields["FeeCurrency"], pRtnOrderField->FeeCurrency);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnTrade:
        {
            auto pRtnTradeField = (CUTRtnTradeField*)data;
            assign(jsonFields["TradeID"], pRtnTradeField->TradeID);
            assign(jsonFields["OrderSysID"], pRtnTradeField->OrderSysID);
            assign(jsonFields["ExchangeID"], pRtnTradeField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnTradeField->InstrumentID);
            assign(jsonFields["MatchPrice"], pRtnTradeField->MatchPrice);
            assign(jsonFields["MatchVolume"], pRtnTradeField->MatchVolume);
            assign(jsonFields["MatchValue"], pRtnTradeField->MatchValue);
            assign(jsonFields["Direction"], pRtnTradeField->Direction);
            assign(jsonFields["OrderLocalID"], pRtnTradeField->OrderLocalID);
            assign(jsonFields["Fee"], pRtnTradeField->Fee);
            assign(jsonFields["FeeCurrency"], pRtnTradeField->FeeCurrency);
            assign(jsonFields["PlatformTime"], pRtnTradeField->PlatformTime);
            assign(jsonFields["TradeTime"], pRtnTradeField->TradeTime);
            assign(jsonFields["RspLocalTime"], pRtnTradeField->RspLocalTime);
            assign(jsonFields["Price"], pRtnTradeField->Price);
            assign(jsonFields["StrategyOrderID"], pRtnTradeField->StrategyOrderID);
            assign(jsonFields["OrderMaker"], pRtnTradeField->OrderMaker);
            assign(jsonFields["AssetType"], pRtnTradeField->AssetType);
            assign(jsonFields["TradeChannel"], pRtnTradeField->TradeChannel);
            assign(jsonFields["SessionID"], pRtnTradeField->SessionID);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqCancelOrder:
        {
            auto pReqCancelOrderField = (CUTReqCancelOrderField*)data;
            assign(jsonFields["OrderLocalID"], pReqCancelOrderField->OrderLocalID);
            assign(jsonFields["OrderForeID"], pReqCancelOrderField->OrderForeID);
            assign(jsonFields["OrderSysID"], pReqCancelOrderField->OrderSysID);
            assign(jsonFields["ExchangeID"], pReqCancelOrderField->ExchangeID);
            assign(jsonFields["StrategyOrderID"], pReqCancelOrderField->StrategyOrderID);
            assign(jsonFields["TradeChannel"], pReqCancelOrderField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspCancelOrder:
        {
            auto pRspCancelOrderField = (CUTRspCancelOrderField*)data;
            assign(jsonFields["ExchangeID"], pRspCancelOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRspCancelOrderField->InstrumentID);
            assign(jsonFields["Price"], pRspCancelOrderField->Price);
            assign(jsonFields["Volume"], pRspCancelOrderField->Volume);
            assign(jsonFields["Direction"], pRspCancelOrderField->Direction);
            assign(jsonFields["OffsetFlag"], pRspCancelOrderField->OffsetFlag);
            assign(jsonFields["OrderLocalID"], pRspCancelOrderField->OrderLocalID);
            assign(jsonFields["OrderMaker"], pRspCancelOrderField->OrderMaker);
            assign(jsonFields["OrderType"], pRspCancelOrderField->OrderType);
            assign(jsonFields["LandTime"], pRspCancelOrderField->LandTime);
            assign(jsonFields["SendTime"], pRspCancelOrderField->SendTime);
            assign(jsonFields["StrategyOrderID"], pRspCancelOrderField->StrategyOrderID);
            assign(jsonFields["OrderMode"], pRspCancelOrderField->OrderMode);
            assign(jsonFields["AssetType"], pRspCancelOrderField->AssetType);
            assign(jsonFields["TradeChannel"], pRspCancelOrderField->TradeChannel);
            assign(jsonFields["OrderXO"], pRspCancelOrderField->OrderXO);
            assign(jsonFields["PlatformTime"], pRspCancelOrderField->PlatformTime);
            assign(jsonFields["OrderSysID"], pRspCancelOrderField->OrderSysID);
            assign(jsonFields["OrderForeID"], pRspCancelOrderField->OrderForeID);
            assign(jsonFields["CreateTime"], pRspCancelOrderField->CreateTime);
            assign(jsonFields["ModifyTime"], pRspCancelOrderField->ModifyTime);
            assign(jsonFields["RspLocalTime"], pRspCancelOrderField->RspLocalTime);
            assign(jsonFields["Cost"], pRspCancelOrderField->Cost);
            assign(jsonFields["TradePrice"], pRspCancelOrderField->TradePrice);
            assign(jsonFields["TradeVolume"], pRspCancelOrderField->TradeVolume);
            assign(jsonFields["TradeValue"], pRspCancelOrderField->TradeValue);
            assign(jsonFields["OrderStatus"], pRspCancelOrderField->OrderStatus);
            assign(jsonFields["SessionID"], pRspCancelOrderField->SessionID);
            assign(jsonFields["RequestID"], pRspCancelOrderField->RequestID);
            assign(jsonFields["RequestForeID"], pRspCancelOrderField->RequestForeID);
            assign(jsonFields["Fee"], pRspCancelOrderField->Fee);
            assign(jsonFields["FeeCurrency"], pRspCancelOrderField->FeeCurrency);
            return jsonFields.dump();
        }
        break;
        case UT_FID_SubPosition:
        {
            auto pSubPositionField = (CUTSubPositionField*)data;
            assign(jsonFields["ExchangeID"], pSubPositionField->ExchangeID);
            assign(jsonFields["InstrumentID"], pSubPositionField->InstrumentID);
            assign(jsonFields["AssetType"], pSubPositionField->AssetType);
            assign(jsonFields["PosiDirection"], pSubPositionField->PosiDirection);
            assign(jsonFields["TradeChannel"], pSubPositionField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnPosition:
        {
            auto pRtnPositionField = (CUTRtnPositionField*)data;
            assign(jsonFields["ExchangeID"], pRtnPositionField->ExchangeID);
            assign(jsonFields["AccountName"], pRtnPositionField->AccountName);
            assign(jsonFields["AccountType"], pRtnPositionField->AccountType);
            assign(jsonFields["InstrumentID"], pRtnPositionField->InstrumentID);
            assign(jsonFields["PosiDirection"], pRtnPositionField->PosiDirection);
            assign(jsonFields["Position"], pRtnPositionField->Position);
            assign(jsonFields["YDPosition"], pRtnPositionField->YDPosition);
            assign(jsonFields["Price"], pRtnPositionField->Price);
            assign(jsonFields["Frozen"], pRtnPositionField->Frozen);
            assign(jsonFields["Available"], pRtnPositionField->Available);
            assign(jsonFields["TotalAvail"], pRtnPositionField->TotalAvail);
            assign(jsonFields["UpdateTime"], pRtnPositionField->UpdateTime);
            assign(jsonFields["CreateTime"], pRtnPositionField->CreateTime);
            assign(jsonFields["CurrencyID"], pRtnPositionField->CurrencyID);
            assign(jsonFields["BaseCurrency"], pRtnPositionField->BaseCurrency);
            assign(jsonFields["OrderMargin"], pRtnPositionField->OrderMargin);
            assign(jsonFields["PositionMargin"], pRtnPositionField->PositionMargin);
            assign(jsonFields["FrozenBuy"], pRtnPositionField->FrozenBuy);
            assign(jsonFields["FrozenSell"], pRtnPositionField->FrozenSell);
            assign(jsonFields["PositionID"], pRtnPositionField->PositionID);
            assign(jsonFields["AssetType"], pRtnPositionField->AssetType);
            assign(jsonFields["TradeChannel"], pRtnPositionField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryOrder:
        {
            auto pReqQryOrderField = (CUTReqQryOrderField*)data;
            assign(jsonFields["StrategyOrderID"], pReqQryOrderField->StrategyOrderID);
            assign(jsonFields["OrderSysID"], pReqQryOrderField->OrderSysID);
            assign(jsonFields["ExchangeID"], pReqQryOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pReqQryOrderField->InstrumentID);
            assign(jsonFields["TradeChannel"], pReqQryOrderField->TradeChannel);
            assign(jsonFields["TimeStart"], pReqQryOrderField->TimeStart);
            assign(jsonFields["TimeEnd"], pReqQryOrderField->TimeEnd);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryOrder:
        {
            auto pRspQryOrderField = (CUTRspQryOrderField*)data;
            assign(jsonFields["ExchangeID"], pRspQryOrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRspQryOrderField->InstrumentID);
            assign(jsonFields["Price"], pRspQryOrderField->Price);
            assign(jsonFields["Volume"], pRspQryOrderField->Volume);
            assign(jsonFields["Direction"], pRspQryOrderField->Direction);
            assign(jsonFields["OffsetFlag"], pRspQryOrderField->OffsetFlag);
            assign(jsonFields["OrderLocalID"], pRspQryOrderField->OrderLocalID);
            assign(jsonFields["OrderMaker"], pRspQryOrderField->OrderMaker);
            assign(jsonFields["OrderType"], pRspQryOrderField->OrderType);
            assign(jsonFields["LandTime"], pRspQryOrderField->LandTime);
            assign(jsonFields["SendTime"], pRspQryOrderField->SendTime);
            assign(jsonFields["StrategyOrderID"], pRspQryOrderField->StrategyOrderID);
            assign(jsonFields["OrderMode"], pRspQryOrderField->OrderMode);
            assign(jsonFields["AssetType"], pRspQryOrderField->AssetType);
            assign(jsonFields["TradeChannel"], pRspQryOrderField->TradeChannel);
            assign(jsonFields["OrderXO"], pRspQryOrderField->OrderXO);
            assign(jsonFields["PlatformTime"], pRspQryOrderField->PlatformTime);
            assign(jsonFields["OrderSysID"], pRspQryOrderField->OrderSysID);
            assign(jsonFields["OrderForeID"], pRspQryOrderField->OrderForeID);
            assign(jsonFields["CreateTime"], pRspQryOrderField->CreateTime);
            assign(jsonFields["ModifyTime"], pRspQryOrderField->ModifyTime);
            assign(jsonFields["RspLocalTime"], pRspQryOrderField->RspLocalTime);
            assign(jsonFields["Cost"], pRspQryOrderField->Cost);
            assign(jsonFields["TradePrice"], pRspQryOrderField->TradePrice);
            assign(jsonFields["TradeVolume"], pRspQryOrderField->TradeVolume);
            assign(jsonFields["TradeValue"], pRspQryOrderField->TradeValue);
            assign(jsonFields["OrderStatus"], pRspQryOrderField->OrderStatus);
            assign(jsonFields["SessionID"], pRspQryOrderField->SessionID);
            assign(jsonFields["RequestID"], pRspQryOrderField->RequestID);
            assign(jsonFields["RequestForeID"], pRspQryOrderField->RequestForeID);
            assign(jsonFields["Fee"], pRspQryOrderField->Fee);
            assign(jsonFields["FeeCurrency"], pRspQryOrderField->FeeCurrency);
            assign(jsonFields["TimeStart"], pRspQryOrderField->TimeStart);
            assign(jsonFields["TimeEnd"], pRspQryOrderField->TimeEnd);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryTrade:
        {
            auto pReqQryTradeField = (CUTReqQryTradeField*)data;
            assign(jsonFields["TradeID"], pReqQryTradeField->TradeID);
            assign(jsonFields["TradeChannel"], pReqQryTradeField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryTrade:
        {
            auto pRspQryTradeField = (CUTRspQryTradeField*)data;
            assign(jsonFields["TradeID"], pRspQryTradeField->TradeID);
            assign(jsonFields["OrderSysID"], pRspQryTradeField->OrderSysID);
            assign(jsonFields["ExchangeID"], pRspQryTradeField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRspQryTradeField->InstrumentID);
            assign(jsonFields["MatchPrice"], pRspQryTradeField->MatchPrice);
            assign(jsonFields["MatchVolume"], pRspQryTradeField->MatchVolume);
            assign(jsonFields["MatchValue"], pRspQryTradeField->MatchValue);
            assign(jsonFields["Direction"], pRspQryTradeField->Direction);
            assign(jsonFields["OrderLocalID"], pRspQryTradeField->OrderLocalID);
            assign(jsonFields["Fee"], pRspQryTradeField->Fee);
            assign(jsonFields["FeeCurrency"], pRspQryTradeField->FeeCurrency);
            assign(jsonFields["PlatformTime"], pRspQryTradeField->PlatformTime);
            assign(jsonFields["TradeTime"], pRspQryTradeField->TradeTime);
            assign(jsonFields["RspLocalTime"], pRspQryTradeField->RspLocalTime);
            assign(jsonFields["Price"], pRspQryTradeField->Price);
            assign(jsonFields["StrategyOrderID"], pRspQryTradeField->StrategyOrderID);
            assign(jsonFields["OrderMaker"], pRspQryTradeField->OrderMaker);
            assign(jsonFields["AssetType"], pRspQryTradeField->AssetType);
            assign(jsonFields["TradeChannel"], pRspQryTradeField->TradeChannel);
            assign(jsonFields["SessionID"], pRspQryTradeField->SessionID);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryAccount:
        {
            auto pReqQryAccountField = (CUTReqQryAccountField*)data;
            assign(jsonFields["Currency"], pReqQryAccountField->Currency);
            assign(jsonFields["TradeChannel"], pReqQryAccountField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryAccount:
        {
            auto pRspQryAccountField = (CUTRspQryAccountField*)data;
            assign(jsonFields["QueryTime"], pRspQryAccountField->QueryTime);
            assign(jsonFields["ExchangeID"], pRspQryAccountField->ExchangeID);
            assign(jsonFields["Currency"], pRspQryAccountField->Currency);
            assign(jsonFields["PositionMargin"], pRspQryAccountField->PositionMargin);
            assign(jsonFields["OrderMargin"], pRspQryAccountField->OrderMargin);
            assign(jsonFields["Available"], pRspQryAccountField->Available);
            assign(jsonFields["PositionBalance"], pRspQryAccountField->PositionBalance);
            assign(jsonFields["TotalAsset"], pRspQryAccountField->TotalAsset);
            assign(jsonFields["TradeChannel"], pRspQryAccountField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_SubAccount:
        {
            auto pSubAccountField = (CUTSubAccountField*)data;
            assign(jsonFields["Currency"], pSubAccountField->Currency);
            assign(jsonFields["AssetType"], pSubAccountField->AssetType);
            assign(jsonFields["TradeChannel"], pSubAccountField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnAccount:
        {
            auto pRtnAccountField = (CUTRtnAccountField*)data;
            assign(jsonFields["ExchangeID"], pRtnAccountField->ExchangeID);
            assign(jsonFields["AccountName"], pRtnAccountField->AccountName);
            assign(jsonFields["AccountType"], pRtnAccountField->AccountType);
            assign(jsonFields["CurrencyName"], pRtnAccountField->CurrencyName);
            assign(jsonFields["CurrencyQuantity"], pRtnAccountField->CurrencyQuantity);
            assign(jsonFields["PositionMargin"], pRtnAccountField->PositionMargin);
            assign(jsonFields["OrderMargin"], pRtnAccountField->OrderMargin);
            assign(jsonFields["PositionBalance"], pRtnAccountField->PositionBalance);
            assign(jsonFields["TotalBalance"], pRtnAccountField->TotalBalance);
            assign(jsonFields["Available"], pRtnAccountField->Available);
            assign(jsonFields["LongAvailable"], pRtnAccountField->LongAvailable);
            assign(jsonFields["ShortAvailable"], pRtnAccountField->ShortAvailable);
            assign(jsonFields["ActualLongAvail"], pRtnAccountField->ActualLongAvail);
            assign(jsonFields["ActualShortAvail"], pRtnAccountField->ActualShortAvail);
            assign(jsonFields["Frozen"], pRtnAccountField->Frozen);
            assign(jsonFields["Fee"], pRtnAccountField->Fee);
            assign(jsonFields["FrozenBuy"], pRtnAccountField->FrozenBuy);
            assign(jsonFields["FrozenSell"], pRtnAccountField->FrozenSell);
            assign(jsonFields["UpdateTime"], pRtnAccountField->UpdateTime);
            assign(jsonFields["CurrencyID"], pRtnAccountField->CurrencyID);
            assign(jsonFields["AssetType"], pRtnAccountField->AssetType);
            assign(jsonFields["TradeChannel"], pRtnAccountField->TradeChannel);
            assign(jsonFields["Borrow"], pRtnAccountField->Borrow);
            assign(jsonFields["Lend"], pRtnAccountField->Lend);
            assign(jsonFields["DebtOffset"], pRtnAccountField->DebtOffset);
            assign(jsonFields["TransferOffset"], pRtnAccountField->TransferOffset);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqLogin:
        {
            auto pReqLoginField = (CUTReqLoginField*)data;
            assign(jsonFields["ClientType"], pReqLoginField->ClientType);
            assign(jsonFields["UserName"], pReqLoginField->UserName);
            assign(jsonFields["ClientName"], pReqLoginField->ClientName);
            assign(jsonFields["Password"], pReqLoginField->Password);
            assign(jsonFields["ReqSequenceID"], pReqLoginField->ReqSequenceID);
            assign(jsonFields["ApiKey"], pReqLoginField->ApiKey);
            assign(jsonFields["ApiSecret"], pReqLoginField->ApiSecret);
            assign(jsonFields["ApiPassword"], pReqLoginField->ApiPassword);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspLogin:
        {
            auto pRspLoginField = (CUTRspLoginField*)data;
            assign(jsonFields["ClientType"], pRspLoginField->ClientType);
            assign(jsonFields["UserName"], pRspLoginField->UserName);
            assign(jsonFields["Password"], pRspLoginField->Password);
            assign(jsonFields["ServerTime"], pRspLoginField->ServerTime);
            assign(jsonFields["TimeB4Launch"], pRspLoginField->TimeB4Launch);
            assign(jsonFields["RspSequenceID"], pRspLoginField->RspSequenceID);
            assign(jsonFields["StrategyName"], pRspLoginField->StrategyName);
            assign(jsonFields["AccessToken"], pRspLoginField->AccessToken);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqLogout:
        {
            auto pReqLogoutField = (CUTReqLogoutField*)data;
            assign(jsonFields["UserName"], pReqLogoutField->UserName);
            assign(jsonFields["AccountName"], pReqLogoutField->AccountName);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspLogout:
        {
            auto pRspLogoutField = (CUTRspLogoutField*)data;
            assign(jsonFields["UserName"], pRspLogoutField->UserName);
            assign(jsonFields["ServerTime"], pRspLogoutField->ServerTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryPosition:
        {
            auto pReqQryPositionField = (CUTReqQryPositionField*)data;
            assign(jsonFields["ExchangeID"], pReqQryPositionField->ExchangeID);
            assign(jsonFields["InstrumentID"], pReqQryPositionField->InstrumentID);
            assign(jsonFields["AccessToken"], pReqQryPositionField->AccessToken);
            assign(jsonFields["TradeChannel"], pReqQryPositionField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryPosition:
        {
            auto pRspQryPositionField = (CUTRspQryPositionField*)data;
            assign(jsonFields["ExchangeID"], pRspQryPositionField->ExchangeID);
            assign(jsonFields["AccountName"], pRspQryPositionField->AccountName);
            assign(jsonFields["AccountType"], pRspQryPositionField->AccountType);
            assign(jsonFields["InstrumentID"], pRspQryPositionField->InstrumentID);
            assign(jsonFields["PosiDirection"], pRspQryPositionField->PosiDirection);
            assign(jsonFields["Position"], pRspQryPositionField->Position);
            assign(jsonFields["YDPosition"], pRspQryPositionField->YDPosition);
            assign(jsonFields["Price"], pRspQryPositionField->Price);
            assign(jsonFields["Frozen"], pRspQryPositionField->Frozen);
            assign(jsonFields["Available"], pRspQryPositionField->Available);
            assign(jsonFields["TotalAvail"], pRspQryPositionField->TotalAvail);
            assign(jsonFields["UpdateTime"], pRspQryPositionField->UpdateTime);
            assign(jsonFields["CreateTime"], pRspQryPositionField->CreateTime);
            assign(jsonFields["CurrencyID"], pRspQryPositionField->CurrencyID);
            assign(jsonFields["BaseCurrency"], pRspQryPositionField->BaseCurrency);
            assign(jsonFields["OrderMargin"], pRspQryPositionField->OrderMargin);
            assign(jsonFields["PositionMargin"], pRspQryPositionField->PositionMargin);
            assign(jsonFields["FrozenBuy"], pRspQryPositionField->FrozenBuy);
            assign(jsonFields["FrozenSell"], pRspQryPositionField->FrozenSell);
            assign(jsonFields["PositionID"], pRspQryPositionField->PositionID);
            assign(jsonFields["AssetType"], pRspQryPositionField->AssetType);
            assign(jsonFields["TradeChannel"], pRspQryPositionField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnPlatformDetail:
        {
            auto pRtnPlatformDetailField = (CUTRtnPlatformDetailField*)data;
            assign(jsonFields["Active"], pRtnPlatformDetailField->Active);
            assign(jsonFields["UpdateTime"], pRtnPlatformDetailField->UpdateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnStrategyDetail:
        {
            auto pRtnStrategyDetailField = (CUTRtnStrategyDetailField*)data;
            assign(jsonFields["Active"], pRtnStrategyDetailField->Active);
            assign(jsonFields["StrategyName"], pRtnStrategyDetailField->StrategyName);
            assign(jsonFields["UpdateTime"], pRtnStrategyDetailField->UpdateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnBusinessDebt:
        {
            auto pRtnBusinessDebtField = (CUTRtnBusinessDebtField*)data;
            assign(jsonFields["ExchangeID"], pRtnBusinessDebtField->ExchangeID);
            assign(jsonFields["AccountName"], pRtnBusinessDebtField->AccountName);
            assign(jsonFields["AccountType"], pRtnBusinessDebtField->AccountType);
            assign(jsonFields["CurrencyName"], pRtnBusinessDebtField->CurrencyName);
            assign(jsonFields["CurrBusinessName"], pRtnBusinessDebtField->CurrBusinessName);
            assign(jsonFields["DebtBusinessName"], pRtnBusinessDebtField->DebtBusinessName);
            assign(jsonFields["DebtDirection"], pRtnBusinessDebtField->DebtDirection);
            assign(jsonFields["DebtAmount"], pRtnBusinessDebtField->DebtAmount);
            assign(jsonFields["CreateTime"], pRtnBusinessDebtField->CreateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryAccountBusiness:
        {
            auto pReqQryAccountBusinessField = (CUTReqQryAccountBusinessField*)data;
            assign(jsonFields["Currency"], pReqQryAccountBusinessField->Currency);
            assign(jsonFields["TradeChannel"], pReqQryAccountBusinessField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryAccountBusiness:
        {
            auto pRspQryAccountBusinessField = (CUTRspQryAccountBusinessField*)data;
            assign(jsonFields["QueryTime"], pRspQryAccountBusinessField->QueryTime);
            assign(jsonFields["ExchangeID"], pRspQryAccountBusinessField->ExchangeID);
            assign(jsonFields["Currency"], pRspQryAccountBusinessField->Currency);
            assign(jsonFields["PositionMargin"], pRspQryAccountBusinessField->PositionMargin);
            assign(jsonFields["OrderMargin"], pRspQryAccountBusinessField->OrderMargin);
            assign(jsonFields["Available"], pRspQryAccountBusinessField->Available);
            assign(jsonFields["PositionBalance"], pRspQryAccountBusinessField->PositionBalance);
            assign(jsonFields["TotalAsset"], pRspQryAccountBusinessField->TotalAsset);
            assign(jsonFields["TradeChannel"], pRspQryAccountBusinessField->TradeChannel);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnAccountBusiness:
        {
            auto pRtnAccountBusinessField = (CUTRtnAccountBusinessField*)data;
            assign(jsonFields["ExchangeID"], pRtnAccountBusinessField->ExchangeID);
            assign(jsonFields["AccountName"], pRtnAccountBusinessField->AccountName);
            assign(jsonFields["AccountType"], pRtnAccountBusinessField->AccountType);
            assign(jsonFields["CurrencyName"], pRtnAccountBusinessField->CurrencyName);
            assign(jsonFields["CurrencyQuantity"], pRtnAccountBusinessField->CurrencyQuantity);
            assign(jsonFields["PositionMargin"], pRtnAccountBusinessField->PositionMargin);
            assign(jsonFields["OrderMargin"], pRtnAccountBusinessField->OrderMargin);
            assign(jsonFields["PositionBalance"], pRtnAccountBusinessField->PositionBalance);
            assign(jsonFields["TotalBalance"], pRtnAccountBusinessField->TotalBalance);
            assign(jsonFields["Available"], pRtnAccountBusinessField->Available);
            assign(jsonFields["LongAvailable"], pRtnAccountBusinessField->LongAvailable);
            assign(jsonFields["ShortAvailable"], pRtnAccountBusinessField->ShortAvailable);
            assign(jsonFields["ActualLongAvail"], pRtnAccountBusinessField->ActualLongAvail);
            assign(jsonFields["ActualShortAvail"], pRtnAccountBusinessField->ActualShortAvail);
            assign(jsonFields["Frozen"], pRtnAccountBusinessField->Frozen);
            assign(jsonFields["Fee"], pRtnAccountBusinessField->Fee);
            assign(jsonFields["FrozenBuy"], pRtnAccountBusinessField->FrozenBuy);
            assign(jsonFields["FrozenSell"], pRtnAccountBusinessField->FrozenSell);
            assign(jsonFields["UpdateTime"], pRtnAccountBusinessField->UpdateTime);
            assign(jsonFields["CurrencyID"], pRtnAccountBusinessField->CurrencyID);
            assign(jsonFields["AssetType"], pRtnAccountBusinessField->AssetType);
            assign(jsonFields["TradeChannel"], pRtnAccountBusinessField->TradeChannel);
            assign(jsonFields["Borrow"], pRtnAccountBusinessField->Borrow);
            assign(jsonFields["Lend"], pRtnAccountBusinessField->Lend);
            assign(jsonFields["DebtOffset"], pRtnAccountBusinessField->DebtOffset);
            assign(jsonFields["TransferOffset"], pRtnAccountBusinessField->TransferOffset);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqManualTransact:
        {
            auto pReqManualTransactField = (CUTReqManualTransactField*)data;
            assign(jsonFields["EventID"], pReqManualTransactField->EventID);
            assign(jsonFields["ExchangeID"], pReqManualTransactField->ExchangeID);
            assign(jsonFields["AccountName"], pReqManualTransactField->AccountName);
            assign(jsonFields["BusinessName"], pReqManualTransactField->BusinessName);
            assign(jsonFields["AccountType"], pReqManualTransactField->AccountType);
            assign(jsonFields["TransactDirection"], pReqManualTransactField->TransactDirection);
            assign(jsonFields["Currency"], pReqManualTransactField->Currency);
            assign(jsonFields["Amount"], pReqManualTransactField->Amount);
            assign(jsonFields["CreateTime"], pReqManualTransactField->CreateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqTransact:
        {
            auto pReqTransactField = (CUTReqTransactField*)data;
            assign(jsonFields["EventID"], pReqTransactField->EventID);
            assign(jsonFields["AccountNameFrom"], pReqTransactField->AccountNameFrom);
            assign(jsonFields["AccountNameTo"], pReqTransactField->AccountNameTo);
            assign(jsonFields["Currency"], pReqTransactField->Currency);
            assign(jsonFields["Direction"], pReqTransactField->Direction);
            assign(jsonFields["Amount"], pReqTransactField->Amount);
            assign(jsonFields["TradeChannelFrom"], pReqTransactField->TradeChannelFrom);
            assign(jsonFields["TradeChannelTo"], pReqTransactField->TradeChannelTo);
            assign(jsonFields["Type"], pReqTransactField->Type);
            assign(jsonFields["Address"], pReqTransactField->Address);
            assign(jsonFields["AddMemo"], pReqTransactField->AddMemo);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspTransact:
        {
            auto pRspTransactField = (CUTRspTransactField*)data;
            assign(jsonFields["EventID"], pRspTransactField->EventID);
            assign(jsonFields["AccountNameFrom"], pRspTransactField->AccountNameFrom);
            assign(jsonFields["AccountNameTo"], pRspTransactField->AccountNameTo);
            assign(jsonFields["Currency"], pRspTransactField->Currency);
            assign(jsonFields["Direction"], pRspTransactField->Direction);
            assign(jsonFields["Amount"], pRspTransactField->Amount);
            assign(jsonFields["TradeChannelFrom"], pRspTransactField->TradeChannelFrom);
            assign(jsonFields["TradeChannelTo"], pRspTransactField->TradeChannelTo);
            assign(jsonFields["Type"], pRspTransactField->Type);
            assign(jsonFields["Address"], pRspTransactField->Address);
            assign(jsonFields["AddMemo"], pRspTransactField->AddMemo);
            assign(jsonFields["ID"], pRspTransactField->ID);
            assign(jsonFields["CreateTime"], pRspTransactField->CreateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnTransact:
        {
            auto pRtnTransactField = (CUTRtnTransactField*)data;
            assign(jsonFields["EventID"], pRtnTransactField->EventID);
            assign(jsonFields["AccountNameFrom"], pRtnTransactField->AccountNameFrom);
            assign(jsonFields["AccountNameTo"], pRtnTransactField->AccountNameTo);
            assign(jsonFields["Currency"], pRtnTransactField->Currency);
            assign(jsonFields["Direction"], pRtnTransactField->Direction);
            assign(jsonFields["Amount"], pRtnTransactField->Amount);
            assign(jsonFields["TradeChannelFrom"], pRtnTransactField->TradeChannelFrom);
            assign(jsonFields["TradeChannelTo"], pRtnTransactField->TradeChannelTo);
            assign(jsonFields["Type"], pRtnTransactField->Type);
            assign(jsonFields["Address"], pRtnTransactField->Address);
            assign(jsonFields["AddMemo"], pRtnTransactField->AddMemo);
            assign(jsonFields["ID"], pRtnTransactField->ID);
            assign(jsonFields["SessionID"], pRtnTransactField->SessionID);
            assign(jsonFields["TransactStatus"], pRtnTransactField->TransactStatus);
            assign(jsonFields["Fee"], pRtnTransactField->Fee);
            assign(jsonFields["FeeCurrency"], pRtnTransactField->FeeCurrency);
            assign(jsonFields["CreateTime"], pRtnTransactField->CreateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_ReqQryTransact:
        {
            auto pReqQryTransactField = (CUTReqQryTransactField*)data;
            assign(jsonFields["EventID"], pReqQryTransactField->EventID);
            assign(jsonFields["AccountNameFrom"], pReqQryTransactField->AccountNameFrom);
            assign(jsonFields["AccountNameTo"], pReqQryTransactField->AccountNameTo);
            assign(jsonFields["Direction"], pReqQryTransactField->Direction);
            assign(jsonFields["Currency"], pReqQryTransactField->Currency);
            assign(jsonFields["TradeChannelFrom"], pReqQryTransactField->TradeChannelFrom);
            assign(jsonFields["TradeChannelTo"], pReqQryTransactField->TradeChannelTo);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RspQryTransact:
        {
            auto pRspQryTransactField = (CUTRspQryTransactField*)data;
            assign(jsonFields["EventID"], pRspQryTransactField->EventID);
            assign(jsonFields["AccountNameFrom"], pRspQryTransactField->AccountNameFrom);
            assign(jsonFields["AccountNameTo"], pRspQryTransactField->AccountNameTo);
            assign(jsonFields["Currency"], pRspQryTransactField->Currency);
            assign(jsonFields["Direction"], pRspQryTransactField->Direction);
            assign(jsonFields["Amount"], pRspQryTransactField->Amount);
            assign(jsonFields["TradeChannelFrom"], pRspQryTransactField->TradeChannelFrom);
            assign(jsonFields["TradeChannelTo"], pRspQryTransactField->TradeChannelTo);
            assign(jsonFields["Type"], pRspQryTransactField->Type);
            assign(jsonFields["Address"], pRspQryTransactField->Address);
            assign(jsonFields["AddMemo"], pRspQryTransactField->AddMemo);
            assign(jsonFields["ID"], pRspQryTransactField->ID);
            assign(jsonFields["SessionID"], pRspQryTransactField->SessionID);
            assign(jsonFields["TransactStatus"], pRspQryTransactField->TransactStatus);
            assign(jsonFields["Fee"], pRspQryTransactField->Fee);
            assign(jsonFields["FeeCurrency"], pRspQryTransactField->FeeCurrency);
            assign(jsonFields["CreateTime"], pRspQryTransactField->CreateTime);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnDepth:
        {
            auto pRtnDepthField = (CUTRtnDepthField*)data;
            assign(jsonFields["TradingDay"], pRtnDepthField->TradingDay);
            assign(jsonFields["ExchangeID"], pRtnDepthField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnDepthField->InstrumentID);
            assign(jsonFields["ExchangeTime"], pRtnDepthField->ExchangeTime);
            assign(jsonFields["LocalTime"], pRtnDepthField->LocalTime);
            assign(jsonFields["ArriveTime"], pRtnDepthField->ArriveTime);
            assign(jsonFields["PlatformTime"], pRtnDepthField->PlatformTime);
            assign(jsonFields["AskPrice1"], pRtnDepthField->AskPrice1);
            assign(jsonFields["AskVolume1"], pRtnDepthField->AskVolume1);
            assign(jsonFields["BidPrice1"], pRtnDepthField->BidPrice1);
            assign(jsonFields["BidVolume1"], pRtnDepthField->BidVolume1);
            assign(jsonFields["AskPrice2"], pRtnDepthField->AskPrice2);
            assign(jsonFields["AskVolume2"], pRtnDepthField->AskVolume2);
            assign(jsonFields["BidPrice2"], pRtnDepthField->BidPrice2);
            assign(jsonFields["BidVolume2"], pRtnDepthField->BidVolume2);
            assign(jsonFields["AskPrice3"], pRtnDepthField->AskPrice3);
            assign(jsonFields["AskVolume3"], pRtnDepthField->AskVolume3);
            assign(jsonFields["BidPrice3"], pRtnDepthField->BidPrice3);
            assign(jsonFields["BidVolume3"], pRtnDepthField->BidVolume3);
            assign(jsonFields["AskPrice4"], pRtnDepthField->AskPrice4);
            assign(jsonFields["AskVolume4"], pRtnDepthField->AskVolume4);
            assign(jsonFields["BidPrice4"], pRtnDepthField->BidPrice4);
            assign(jsonFields["BidVolume4"], pRtnDepthField->BidVolume4);
            assign(jsonFields["AskPrice5"], pRtnDepthField->AskPrice5);
            assign(jsonFields["AskVolume5"], pRtnDepthField->AskVolume5);
            assign(jsonFields["BidPrice5"], pRtnDepthField->BidPrice5);
            assign(jsonFields["BidVolume5"], pRtnDepthField->BidVolume5);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnL2Depth:
        {
            auto pRtnL2DepthField = (CUTRtnL2DepthField*)data;
            assign(jsonFields["TradingDay"], pRtnL2DepthField->TradingDay);
            assign(jsonFields["ExchangeID"], pRtnL2DepthField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnL2DepthField->InstrumentID);
            assign(jsonFields["ExchangeTime"], pRtnL2DepthField->ExchangeTime);
            assign(jsonFields["LocalTime"], pRtnL2DepthField->LocalTime);
            assign(jsonFields["ArriveTime"], pRtnL2DepthField->ArriveTime);
            assign(jsonFields["PlatformTime"], pRtnL2DepthField->PlatformTime);
            assign(jsonFields["AskPrice1"], pRtnL2DepthField->AskPrice1);
            assign(jsonFields["AskVolume1"], pRtnL2DepthField->AskVolume1);
            assign(jsonFields["BidPrice1"], pRtnL2DepthField->BidPrice1);
            assign(jsonFields["BidVolume1"], pRtnL2DepthField->BidVolume1);
            assign(jsonFields["AskPrice2"], pRtnL2DepthField->AskPrice2);
            assign(jsonFields["AskVolume2"], pRtnL2DepthField->AskVolume2);
            assign(jsonFields["BidPrice2"], pRtnL2DepthField->BidPrice2);
            assign(jsonFields["BidVolume2"], pRtnL2DepthField->BidVolume2);
            assign(jsonFields["AskPrice3"], pRtnL2DepthField->AskPrice3);
            assign(jsonFields["AskVolume3"], pRtnL2DepthField->AskVolume3);
            assign(jsonFields["BidPrice3"], pRtnL2DepthField->BidPrice3);
            assign(jsonFields["BidVolume3"], pRtnL2DepthField->BidVolume3);
            assign(jsonFields["AskPrice4"], pRtnL2DepthField->AskPrice4);
            assign(jsonFields["AskVolume4"], pRtnL2DepthField->AskVolume4);
            assign(jsonFields["BidPrice4"], pRtnL2DepthField->BidPrice4);
            assign(jsonFields["BidVolume4"], pRtnL2DepthField->BidVolume4);
            assign(jsonFields["AskPrice5"], pRtnL2DepthField->AskPrice5);
            assign(jsonFields["AskVolume5"], pRtnL2DepthField->AskVolume5);
            assign(jsonFields["BidPrice5"], pRtnL2DepthField->BidPrice5);
            assign(jsonFields["BidVolume5"], pRtnL2DepthField->BidVolume5);
            assign(jsonFields["AskPrice6"], pRtnL2DepthField->AskPrice6);
            assign(jsonFields["AskVolume6"], pRtnL2DepthField->AskVolume6);
            assign(jsonFields["BidPrice6"], pRtnL2DepthField->BidPrice6);
            assign(jsonFields["BidVolume6"], pRtnL2DepthField->BidVolume6);
            assign(jsonFields["AskPrice7"], pRtnL2DepthField->AskPrice7);
            assign(jsonFields["AskVolume7"], pRtnL2DepthField->AskVolume7);
            assign(jsonFields["BidPrice7"], pRtnL2DepthField->BidPrice7);
            assign(jsonFields["BidVolume7"], pRtnL2DepthField->BidVolume7);
            assign(jsonFields["AskPrice8"], pRtnL2DepthField->AskPrice8);
            assign(jsonFields["AskVolume8"], pRtnL2DepthField->AskVolume8);
            assign(jsonFields["BidPrice8"], pRtnL2DepthField->BidPrice8);
            assign(jsonFields["BidVolume8"], pRtnL2DepthField->BidVolume8);
            assign(jsonFields["AskPrice9"], pRtnL2DepthField->AskPrice9);
            assign(jsonFields["AskVolume9"], pRtnL2DepthField->AskVolume9);
            assign(jsonFields["BidPrice9"], pRtnL2DepthField->BidPrice9);
            assign(jsonFields["BidVolume9"], pRtnL2DepthField->BidVolume9);
            assign(jsonFields["AskPrice10"], pRtnL2DepthField->AskPrice10);
            assign(jsonFields["AskVolume10"], pRtnL2DepthField->AskVolume10);
            assign(jsonFields["BidPrice10"], pRtnL2DepthField->BidPrice10);
            assign(jsonFields["BidVolume10"], pRtnL2DepthField->BidVolume10);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnL2Trade:
        {
            auto pRtnL2TradeField = (CUTRtnL2TradeField*)data;
            assign(jsonFields["ExchangeTime"], pRtnL2TradeField->ExchangeTime);
            assign(jsonFields["LocalTime"], pRtnL2TradeField->LocalTime);
            assign(jsonFields["ArriveTime"], pRtnL2TradeField->ArriveTime);
            assign(jsonFields["PlatformTime"], pRtnL2TradeField->PlatformTime);
            assign(jsonFields["ExchangeID"], pRtnL2TradeField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnL2TradeField->InstrumentID);
            assign(jsonFields["LastPrice"], pRtnL2TradeField->LastPrice);
            assign(jsonFields["Side"], pRtnL2TradeField->Side);
            assign(jsonFields["CeilingPrice"], pRtnL2TradeField->CeilingPrice);
            assign(jsonFields["FloorPrice"], pRtnL2TradeField->FloorPrice);
            assign(jsonFields["BestAsk"], pRtnL2TradeField->BestAsk);
            assign(jsonFields["BestBid"], pRtnL2TradeField->BestBid);
            assign(jsonFields["Quantity"], pRtnL2TradeField->Quantity);
            assign(jsonFields["TotalQuantity"], pRtnL2TradeField->TotalQuantity);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnL2Order:
        {
            auto pRtnL2OrderField = (CUTRtnL2OrderField*)data;
            assign(jsonFields["ExchangeTime"], pRtnL2OrderField->ExchangeTime);
            assign(jsonFields["LocalTime"], pRtnL2OrderField->LocalTime);
            assign(jsonFields["ArriveTime"], pRtnL2OrderField->ArriveTime);
            assign(jsonFields["PlatformTime"], pRtnL2OrderField->PlatformTime);
            assign(jsonFields["ExchangeID"], pRtnL2OrderField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnL2OrderField->InstrumentID);
            assign(jsonFields["Price"], pRtnL2OrderField->Price);
            assign(jsonFields["Volume"], pRtnL2OrderField->Volume);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnL2Index:
        {
            auto pRtnL2IndexField = (CUTRtnL2IndexField*)data;
            assign(jsonFields["ExchangeTime"], pRtnL2IndexField->ExchangeTime);
            assign(jsonFields["LocalTime"], pRtnL2IndexField->LocalTime);
            assign(jsonFields["ArriveTime"], pRtnL2IndexField->ArriveTime);
            assign(jsonFields["PlatformTime"], pRtnL2IndexField->PlatformTime);
            assign(jsonFields["ExchangeID"], pRtnL2IndexField->ExchangeID);
            assign(jsonFields["InstrumentID"], pRtnL2IndexField->InstrumentID);
            assign(jsonFields["PreCloseIndex"], pRtnL2IndexField->PreCloseIndex);
            assign(jsonFields["OpenIndex"], pRtnL2IndexField->OpenIndex);
            assign(jsonFields["CloseIndex"], pRtnL2IndexField->CloseIndex);
            assign(jsonFields["HighIndex"], pRtnL2IndexField->HighIndex);
            assign(jsonFields["LowIndex"], pRtnL2IndexField->LowIndex);
            assign(jsonFields["LastIndex"], pRtnL2IndexField->LastIndex);
            assign(jsonFields["TurnOver"], pRtnL2IndexField->TurnOver);
            assign(jsonFields["TotalVolume"], pRtnL2IndexField->TotalVolume);
            return jsonFields.dump();
        }
        break;
        case UT_FID_RtnBarMarketData:
        {
            auto pRtnBarMarketDataField = (CUTRtnBarMarketDataField*)data;
            assign(jsonFields["TradingDay"], pRtnBarMarketDataField->TradingDay);
            assign(jsonFields["InstrumentID"], pRtnBarMarketDataField->InstrumentID);
            assign(jsonFields["CeilingPrice"], pRtnBarMarketDataField->CeilingPrice);
            assign(jsonFields["FloorPrice"], pRtnBarMarketDataField->FloorPrice);
            assign(jsonFields["StartUpdateTime"], pRtnBarMarketDataField->StartUpdateTime);
            assign(jsonFields["EndUpdateTime"], pRtnBarMarketDataField->EndUpdateTime);
            assign(jsonFields["Open"], pRtnBarMarketDataField->Open);
            assign(jsonFields["Close"], pRtnBarMarketDataField->Close);
            assign(jsonFields["Low"], pRtnBarMarketDataField->Low);
            assign(jsonFields["High"], pRtnBarMarketDataField->High);
            assign(jsonFields["Volume"], pRtnBarMarketDataField->Volume);
            assign(jsonFields["StartVolume"], pRtnBarMarketDataField->StartVolume);
            return jsonFields.dump();
        }
        break;
        default:
        {
            std::stringstream sstream;
            sstream << "Unexcept Type: " << msg_type;
            return sstream.str();
        }
    }
}

inline void Jason2UTData(void* data, short msg_type, std::string content)
{
    nlohmann::json js = nlohmann::json::parse(content);
    switch(msg_type)
    {
        case UT_FID_RspInfo:
        {
            auto pRspInfoField = (CUTRspInfoField*)data;
            assign(pRspInfoField->ErrorID, js["ErrorID"].get<int>());
            assign(pRspInfoField->ErrorMsg, js["ErrorMsg"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqCreateOrder:
        {
            auto pReqCreateOrderField = (CUTReqCreateOrderField*)data;
            assign(pReqCreateOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->Price, js["Price"].get<double>());
            assign(pReqCreateOrderField->Volume, js["Volume"].get<double>());
            assign(pReqCreateOrderField->Direction, js["Direction"].get<char>());
            assign(pReqCreateOrderField->OffsetFlag, js["OffsetFlag"].get<char>());
            assign(pReqCreateOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pReqCreateOrderField->OrderType, js["OrderType"].get<char>());
            assign(pReqCreateOrderField->LandTime, js["LandTime"].get<long>());
            assign(pReqCreateOrderField->SendTime, js["SendTime"].get<long>());
            assign(pReqCreateOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->OrderMode, js["OrderMode"].get<char>());
            assign(pReqCreateOrderField->AssetType, js["AssetType"].get<char>());
            assign(pReqCreateOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pReqCreateOrderField->OrderXO, js["OrderXO"].get<char>());
            assign(pReqCreateOrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pReqCreateOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pReqCreateOrderField->ModifyTime, js["ModifyTime"].get<std::string>().c_str());
            assign(pReqCreateOrderField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pReqCreateOrderField->Cost, js["Cost"].get<double>());
            assign(pReqCreateOrderField->TradePrice, js["TradePrice"].get<double>());
            assign(pReqCreateOrderField->TradeVolume, js["TradeVolume"].get<double>());
            assign(pReqCreateOrderField->TradeValue, js["TradeValue"].get<double>());
            assign(pReqCreateOrderField->OrderStatus, js["OrderStatus"].get<char>());
            assign(pReqCreateOrderField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pReqCreateOrderField->RequestID, js["RequestID"].get<long>());
            assign(pReqCreateOrderField->RequestForeID, js["RequestForeID"].get<long>());
            assign(pReqCreateOrderField->Fee, js["Fee"].get<double>());
            assign(pReqCreateOrderField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspCreateOrder:
        {
            auto pRspCreateOrderField = (CUTRspCreateOrderField*)data;
            assign(pRspCreateOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->Price, js["Price"].get<double>());
            assign(pRspCreateOrderField->Volume, js["Volume"].get<double>());
            assign(pRspCreateOrderField->Direction, js["Direction"].get<char>());
            assign(pRspCreateOrderField->OffsetFlag, js["OffsetFlag"].get<char>());
            assign(pRspCreateOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRspCreateOrderField->OrderType, js["OrderType"].get<char>());
            assign(pRspCreateOrderField->LandTime, js["LandTime"].get<long>());
            assign(pRspCreateOrderField->SendTime, js["SendTime"].get<long>());
            assign(pRspCreateOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->OrderMode, js["OrderMode"].get<char>());
            assign(pRspCreateOrderField->AssetType, js["AssetType"].get<char>());
            assign(pRspCreateOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRspCreateOrderField->OrderXO, js["OrderXO"].get<char>());
            assign(pRspCreateOrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRspCreateOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRspCreateOrderField->ModifyTime, js["ModifyTime"].get<std::string>().c_str());
            assign(pRspCreateOrderField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRspCreateOrderField->Cost, js["Cost"].get<double>());
            assign(pRspCreateOrderField->TradePrice, js["TradePrice"].get<double>());
            assign(pRspCreateOrderField->TradeVolume, js["TradeVolume"].get<double>());
            assign(pRspCreateOrderField->TradeValue, js["TradeValue"].get<double>());
            assign(pRspCreateOrderField->OrderStatus, js["OrderStatus"].get<char>());
            assign(pRspCreateOrderField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRspCreateOrderField->RequestID, js["RequestID"].get<long>());
            assign(pRspCreateOrderField->RequestForeID, js["RequestForeID"].get<long>());
            assign(pRspCreateOrderField->Fee, js["Fee"].get<double>());
            assign(pRspCreateOrderField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnOrder:
        {
            auto pRtnOrderField = (CUTRtnOrderField*)data;
            assign(pRtnOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnOrderField->Price, js["Price"].get<double>());
            assign(pRtnOrderField->Volume, js["Volume"].get<double>());
            assign(pRtnOrderField->Direction, js["Direction"].get<char>());
            assign(pRtnOrderField->OffsetFlag, js["OffsetFlag"].get<char>());
            assign(pRtnOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRtnOrderField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRtnOrderField->OrderType, js["OrderType"].get<char>());
            assign(pRtnOrderField->LandTime, js["LandTime"].get<long>());
            assign(pRtnOrderField->SendTime, js["SendTime"].get<long>());
            assign(pRtnOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRtnOrderField->OrderMode, js["OrderMode"].get<char>());
            assign(pRtnOrderField->AssetType, js["AssetType"].get<char>());
            assign(pRtnOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRtnOrderField->OrderXO, js["OrderXO"].get<char>());
            assign(pRtnOrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRtnOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pRtnOrderField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRtnOrderField->ModifyTime, js["ModifyTime"].get<std::string>().c_str());
            assign(pRtnOrderField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRtnOrderField->Cost, js["Cost"].get<double>());
            assign(pRtnOrderField->TradePrice, js["TradePrice"].get<double>());
            assign(pRtnOrderField->TradeVolume, js["TradeVolume"].get<double>());
            assign(pRtnOrderField->TradeValue, js["TradeValue"].get<double>());
            assign(pRtnOrderField->OrderStatus, js["OrderStatus"].get<char>());
            assign(pRtnOrderField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRtnOrderField->RequestID, js["RequestID"].get<long>());
            assign(pRtnOrderField->RequestForeID, js["RequestForeID"].get<long>());
            assign(pRtnOrderField->Fee, js["Fee"].get<double>());
            assign(pRtnOrderField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnTrade:
        {
            auto pRtnTradeField = (CUTRtnTradeField*)data;
            assign(pRtnTradeField->TradeID, js["TradeID"].get<std::string>().c_str());
            assign(pRtnTradeField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRtnTradeField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnTradeField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnTradeField->MatchPrice, js["MatchPrice"].get<double>());
            assign(pRtnTradeField->MatchVolume, js["MatchVolume"].get<double>());
            assign(pRtnTradeField->MatchValue, js["MatchValue"].get<double>());
            assign(pRtnTradeField->Direction, js["Direction"].get<char>());
            assign(pRtnTradeField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRtnTradeField->Fee, js["Fee"].get<double>());
            assign(pRtnTradeField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
            assign(pRtnTradeField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnTradeField->TradeTime, js["TradeTime"].get<std::string>().c_str());
            assign(pRtnTradeField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRtnTradeField->Price, js["Price"].get<double>());
            assign(pRtnTradeField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRtnTradeField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRtnTradeField->AssetType, js["AssetType"].get<char>());
            assign(pRtnTradeField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRtnTradeField->SessionID, js["SessionID"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqCancelOrder:
        {
            auto pReqCancelOrderField = (CUTReqCancelOrderField*)data;
            assign(pReqCancelOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pReqCancelOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pReqCancelOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pReqCancelOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pReqCancelOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pReqCancelOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspCancelOrder:
        {
            auto pRspCancelOrderField = (CUTRspCancelOrderField*)data;
            assign(pRspCancelOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->Price, js["Price"].get<double>());
            assign(pRspCancelOrderField->Volume, js["Volume"].get<double>());
            assign(pRspCancelOrderField->Direction, js["Direction"].get<char>());
            assign(pRspCancelOrderField->OffsetFlag, js["OffsetFlag"].get<char>());
            assign(pRspCancelOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRspCancelOrderField->OrderType, js["OrderType"].get<char>());
            assign(pRspCancelOrderField->LandTime, js["LandTime"].get<long>());
            assign(pRspCancelOrderField->SendTime, js["SendTime"].get<long>());
            assign(pRspCancelOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->OrderMode, js["OrderMode"].get<char>());
            assign(pRspCancelOrderField->AssetType, js["AssetType"].get<char>());
            assign(pRspCancelOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRspCancelOrderField->OrderXO, js["OrderXO"].get<char>());
            assign(pRspCancelOrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRspCancelOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRspCancelOrderField->ModifyTime, js["ModifyTime"].get<std::string>().c_str());
            assign(pRspCancelOrderField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRspCancelOrderField->Cost, js["Cost"].get<double>());
            assign(pRspCancelOrderField->TradePrice, js["TradePrice"].get<double>());
            assign(pRspCancelOrderField->TradeVolume, js["TradeVolume"].get<double>());
            assign(pRspCancelOrderField->TradeValue, js["TradeValue"].get<double>());
            assign(pRspCancelOrderField->OrderStatus, js["OrderStatus"].get<char>());
            assign(pRspCancelOrderField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRspCancelOrderField->RequestID, js["RequestID"].get<long>());
            assign(pRspCancelOrderField->RequestForeID, js["RequestForeID"].get<long>());
            assign(pRspCancelOrderField->Fee, js["Fee"].get<double>());
            assign(pRspCancelOrderField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
        }
        break;
        case UT_FID_SubPosition:
        {
            auto pSubPositionField = (CUTSubPositionField*)data;
            assign(pSubPositionField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pSubPositionField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pSubPositionField->AssetType, js["AssetType"].get<char>());
            assign(pSubPositionField->PosiDirection, js["PosiDirection"].get<char>());
            assign(pSubPositionField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnPosition:
        {
            auto pRtnPositionField = (CUTRtnPositionField*)data;
            assign(pRtnPositionField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnPositionField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pRtnPositionField->AccountType, js["AccountType"].get<char>());
            assign(pRtnPositionField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnPositionField->PosiDirection, js["PosiDirection"].get<char>());
            assign(pRtnPositionField->Position, js["Position"].get<double>());
            assign(pRtnPositionField->YDPosition, js["YDPosition"].get<double>());
            assign(pRtnPositionField->Price, js["Price"].get<double>());
            assign(pRtnPositionField->Frozen, js["Frozen"].get<double>());
            assign(pRtnPositionField->Available, js["Available"].get<double>());
            assign(pRtnPositionField->TotalAvail, js["TotalAvail"].get<double>());
            assign(pRtnPositionField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
            assign(pRtnPositionField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRtnPositionField->CurrencyID, js["CurrencyID"].get<long>());
            assign(pRtnPositionField->BaseCurrency, js["BaseCurrency"].get<std::string>().c_str());
            assign(pRtnPositionField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRtnPositionField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRtnPositionField->FrozenBuy, js["FrozenBuy"].get<double>());
            assign(pRtnPositionField->FrozenSell, js["FrozenSell"].get<double>());
            assign(pRtnPositionField->PositionID, js["PositionID"].get<long>());
            assign(pRtnPositionField->AssetType, js["AssetType"].get<char>());
            assign(pRtnPositionField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryOrder:
        {
            auto pReqQryOrderField = (CUTReqQryOrderField*)data;
            assign(pReqQryOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pReqQryOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pReqQryOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pReqQryOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pReqQryOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pReqQryOrderField->TimeStart, js["TimeStart"].get<std::string>().c_str());
            assign(pReqQryOrderField->TimeEnd, js["TimeEnd"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryOrder:
        {
            auto pRspQryOrderField = (CUTRspQryOrderField*)data;
            assign(pRspQryOrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspQryOrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRspQryOrderField->Price, js["Price"].get<double>());
            assign(pRspQryOrderField->Volume, js["Volume"].get<double>());
            assign(pRspQryOrderField->Direction, js["Direction"].get<char>());
            assign(pRspQryOrderField->OffsetFlag, js["OffsetFlag"].get<char>());
            assign(pRspQryOrderField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRspQryOrderField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRspQryOrderField->OrderType, js["OrderType"].get<char>());
            assign(pRspQryOrderField->LandTime, js["LandTime"].get<long>());
            assign(pRspQryOrderField->SendTime, js["SendTime"].get<long>());
            assign(pRspQryOrderField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRspQryOrderField->OrderMode, js["OrderMode"].get<char>());
            assign(pRspQryOrderField->AssetType, js["AssetType"].get<char>());
            assign(pRspQryOrderField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRspQryOrderField->OrderXO, js["OrderXO"].get<char>());
            assign(pRspQryOrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRspQryOrderField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRspQryOrderField->OrderForeID, js["OrderForeID"].get<std::string>().c_str());
            assign(pRspQryOrderField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRspQryOrderField->ModifyTime, js["ModifyTime"].get<std::string>().c_str());
            assign(pRspQryOrderField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRspQryOrderField->Cost, js["Cost"].get<double>());
            assign(pRspQryOrderField->TradePrice, js["TradePrice"].get<double>());
            assign(pRspQryOrderField->TradeVolume, js["TradeVolume"].get<double>());
            assign(pRspQryOrderField->TradeValue, js["TradeValue"].get<double>());
            assign(pRspQryOrderField->OrderStatus, js["OrderStatus"].get<char>());
            assign(pRspQryOrderField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRspQryOrderField->RequestID, js["RequestID"].get<long>());
            assign(pRspQryOrderField->RequestForeID, js["RequestForeID"].get<long>());
            assign(pRspQryOrderField->Fee, js["Fee"].get<double>());
            assign(pRspQryOrderField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
            assign(pRspQryOrderField->TimeStart, js["TimeStart"].get<std::string>().c_str());
            assign(pRspQryOrderField->TimeEnd, js["TimeEnd"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryTrade:
        {
            auto pReqQryTradeField = (CUTReqQryTradeField*)data;
            assign(pReqQryTradeField->TradeID, js["TradeID"].get<std::string>().c_str());
            assign(pReqQryTradeField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryTrade:
        {
            auto pRspQryTradeField = (CUTRspQryTradeField*)data;
            assign(pRspQryTradeField->TradeID, js["TradeID"].get<std::string>().c_str());
            assign(pRspQryTradeField->OrderSysID, js["OrderSysID"].get<std::string>().c_str());
            assign(pRspQryTradeField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspQryTradeField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRspQryTradeField->MatchPrice, js["MatchPrice"].get<double>());
            assign(pRspQryTradeField->MatchVolume, js["MatchVolume"].get<double>());
            assign(pRspQryTradeField->MatchValue, js["MatchValue"].get<double>());
            assign(pRspQryTradeField->Direction, js["Direction"].get<char>());
            assign(pRspQryTradeField->OrderLocalID, js["OrderLocalID"].get<std::string>().c_str());
            assign(pRspQryTradeField->Fee, js["Fee"].get<double>());
            assign(pRspQryTradeField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
            assign(pRspQryTradeField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRspQryTradeField->TradeTime, js["TradeTime"].get<std::string>().c_str());
            assign(pRspQryTradeField->RspLocalTime, js["RspLocalTime"].get<std::string>().c_str());
            assign(pRspQryTradeField->Price, js["Price"].get<double>());
            assign(pRspQryTradeField->StrategyOrderID, js["StrategyOrderID"].get<std::string>().c_str());
            assign(pRspQryTradeField->OrderMaker, js["OrderMaker"].get<char>());
            assign(pRspQryTradeField->AssetType, js["AssetType"].get<char>());
            assign(pRspQryTradeField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRspQryTradeField->SessionID, js["SessionID"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryAccount:
        {
            auto pReqQryAccountField = (CUTReqQryAccountField*)data;
            assign(pReqQryAccountField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pReqQryAccountField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryAccount:
        {
            auto pRspQryAccountField = (CUTRspQryAccountField*)data;
            assign(pRspQryAccountField->QueryTime, js["QueryTime"].get<std::string>().c_str());
            assign(pRspQryAccountField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspQryAccountField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pRspQryAccountField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRspQryAccountField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRspQryAccountField->Available, js["Available"].get<double>());
            assign(pRspQryAccountField->PositionBalance, js["PositionBalance"].get<double>());
            assign(pRspQryAccountField->TotalAsset, js["TotalAsset"].get<double>());
            assign(pRspQryAccountField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_SubAccount:
        {
            auto pSubAccountField = (CUTSubAccountField*)data;
            assign(pSubAccountField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pSubAccountField->AssetType, js["AssetType"].get<char>());
            assign(pSubAccountField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnAccount:
        {
            auto pRtnAccountField = (CUTRtnAccountField*)data;
            assign(pRtnAccountField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnAccountField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pRtnAccountField->AccountType, js["AccountType"].get<char>());
            assign(pRtnAccountField->CurrencyName, js["CurrencyName"].get<std::string>().c_str());
            assign(pRtnAccountField->CurrencyQuantity, js["CurrencyQuantity"].get<double>());
            assign(pRtnAccountField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRtnAccountField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRtnAccountField->PositionBalance, js["PositionBalance"].get<double>());
            assign(pRtnAccountField->TotalBalance, js["TotalBalance"].get<double>());
            assign(pRtnAccountField->Available, js["Available"].get<double>());
            assign(pRtnAccountField->LongAvailable, js["LongAvailable"].get<double>());
            assign(pRtnAccountField->ShortAvailable, js["ShortAvailable"].get<double>());
            assign(pRtnAccountField->ActualLongAvail, js["ActualLongAvail"].get<double>());
            assign(pRtnAccountField->ActualShortAvail, js["ActualShortAvail"].get<double>());
            assign(pRtnAccountField->Frozen, js["Frozen"].get<double>());
            assign(pRtnAccountField->Fee, js["Fee"].get<double>());
            assign(pRtnAccountField->FrozenBuy, js["FrozenBuy"].get<double>());
            assign(pRtnAccountField->FrozenSell, js["FrozenSell"].get<double>());
            assign(pRtnAccountField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
            assign(pRtnAccountField->CurrencyID, js["CurrencyID"].get<long>());
            assign(pRtnAccountField->AssetType, js["AssetType"].get<char>());
            assign(pRtnAccountField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRtnAccountField->Borrow, js["Borrow"].get<double>());
            assign(pRtnAccountField->Lend, js["Lend"].get<double>());
            assign(pRtnAccountField->DebtOffset, js["DebtOffset"].get<double>());
            assign(pRtnAccountField->TransferOffset, js["TransferOffset"].get<double>());
        }
        break;
        case UT_FID_ReqLogin:
        {
            auto pReqLoginField = (CUTReqLoginField*)data;
            assign(pReqLoginField->ClientType, js["ClientType"].get<char>());
            assign(pReqLoginField->UserName, js["UserName"].get<std::string>().c_str());
            assign(pReqLoginField->ClientName, js["ClientName"].get<std::string>().c_str());
            assign(pReqLoginField->Password, js["Password"].get<std::string>().c_str());
            assign(pReqLoginField->ReqSequenceID, js["ReqSequenceID"].get<long>());
            assign(pReqLoginField->ApiKey, js["ApiKey"].get<std::string>().c_str());
            assign(pReqLoginField->ApiSecret, js["ApiSecret"].get<std::string>().c_str());
            assign(pReqLoginField->ApiPassword, js["ApiPassword"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspLogin:
        {
            auto pRspLoginField = (CUTRspLoginField*)data;
            assign(pRspLoginField->ClientType, js["ClientType"].get<char>());
            assign(pRspLoginField->UserName, js["UserName"].get<std::string>().c_str());
            assign(pRspLoginField->Password, js["Password"].get<std::string>().c_str());
            assign(pRspLoginField->ServerTime, js["ServerTime"].get<std::string>().c_str());
            assign(pRspLoginField->TimeB4Launch, js["TimeB4Launch"].get<long>());
            assign(pRspLoginField->RspSequenceID, js["RspSequenceID"].get<long>());
            assign(pRspLoginField->StrategyName, js["StrategyName"].get<std::string>().c_str());
            assign(pRspLoginField->AccessToken, js["AccessToken"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqLogout:
        {
            auto pReqLogoutField = (CUTReqLogoutField*)data;
            assign(pReqLogoutField->UserName, js["UserName"].get<std::string>().c_str());
            assign(pReqLogoutField->AccountName, js["AccountName"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspLogout:
        {
            auto pRspLogoutField = (CUTRspLogoutField*)data;
            assign(pRspLogoutField->UserName, js["UserName"].get<std::string>().c_str());
            assign(pRspLogoutField->ServerTime, js["ServerTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryPosition:
        {
            auto pReqQryPositionField = (CUTReqQryPositionField*)data;
            assign(pReqQryPositionField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pReqQryPositionField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pReqQryPositionField->AccessToken, js["AccessToken"].get<std::string>().c_str());
            assign(pReqQryPositionField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryPosition:
        {
            auto pRspQryPositionField = (CUTRspQryPositionField*)data;
            assign(pRspQryPositionField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspQryPositionField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pRspQryPositionField->AccountType, js["AccountType"].get<char>());
            assign(pRspQryPositionField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRspQryPositionField->PosiDirection, js["PosiDirection"].get<char>());
            assign(pRspQryPositionField->Position, js["Position"].get<double>());
            assign(pRspQryPositionField->YDPosition, js["YDPosition"].get<double>());
            assign(pRspQryPositionField->Price, js["Price"].get<double>());
            assign(pRspQryPositionField->Frozen, js["Frozen"].get<double>());
            assign(pRspQryPositionField->Available, js["Available"].get<double>());
            assign(pRspQryPositionField->TotalAvail, js["TotalAvail"].get<double>());
            assign(pRspQryPositionField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
            assign(pRspQryPositionField->CreateTime, js["CreateTime"].get<std::string>().c_str());
            assign(pRspQryPositionField->CurrencyID, js["CurrencyID"].get<long>());
            assign(pRspQryPositionField->BaseCurrency, js["BaseCurrency"].get<std::string>().c_str());
            assign(pRspQryPositionField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRspQryPositionField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRspQryPositionField->FrozenBuy, js["FrozenBuy"].get<double>());
            assign(pRspQryPositionField->FrozenSell, js["FrozenSell"].get<double>());
            assign(pRspQryPositionField->PositionID, js["PositionID"].get<long>());
            assign(pRspQryPositionField->AssetType, js["AssetType"].get<char>());
            assign(pRspQryPositionField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnPlatformDetail:
        {
            auto pRtnPlatformDetailField = (CUTRtnPlatformDetailField*)data;
            assign(pRtnPlatformDetailField->Active, js["Active"].get<char>());
            assign(pRtnPlatformDetailField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnStrategyDetail:
        {
            auto pRtnStrategyDetailField = (CUTRtnStrategyDetailField*)data;
            assign(pRtnStrategyDetailField->Active, js["Active"].get<char>());
            assign(pRtnStrategyDetailField->StrategyName, js["StrategyName"].get<std::string>().c_str());
            assign(pRtnStrategyDetailField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnBusinessDebt:
        {
            auto pRtnBusinessDebtField = (CUTRtnBusinessDebtField*)data;
            assign(pRtnBusinessDebtField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnBusinessDebtField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pRtnBusinessDebtField->AccountType, js["AccountType"].get<char>());
            assign(pRtnBusinessDebtField->CurrencyName, js["CurrencyName"].get<std::string>().c_str());
            assign(pRtnBusinessDebtField->CurrBusinessName, js["CurrBusinessName"].get<std::string>().c_str());
            assign(pRtnBusinessDebtField->DebtBusinessName, js["DebtBusinessName"].get<std::string>().c_str());
            assign(pRtnBusinessDebtField->DebtDirection, js["DebtDirection"].get<char>());
            assign(pRtnBusinessDebtField->DebtAmount, js["DebtAmount"].get<double>());
            assign(pRtnBusinessDebtField->CreateTime, js["CreateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryAccountBusiness:
        {
            auto pReqQryAccountBusinessField = (CUTReqQryAccountBusinessField*)data;
            assign(pReqQryAccountBusinessField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pReqQryAccountBusinessField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryAccountBusiness:
        {
            auto pRspQryAccountBusinessField = (CUTRspQryAccountBusinessField*)data;
            assign(pRspQryAccountBusinessField->QueryTime, js["QueryTime"].get<std::string>().c_str());
            assign(pRspQryAccountBusinessField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRspQryAccountBusinessField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pRspQryAccountBusinessField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRspQryAccountBusinessField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRspQryAccountBusinessField->Available, js["Available"].get<double>());
            assign(pRspQryAccountBusinessField->PositionBalance, js["PositionBalance"].get<double>());
            assign(pRspQryAccountBusinessField->TotalAsset, js["TotalAsset"].get<double>());
            assign(pRspQryAccountBusinessField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnAccountBusiness:
        {
            auto pRtnAccountBusinessField = (CUTRtnAccountBusinessField*)data;
            assign(pRtnAccountBusinessField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnAccountBusinessField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pRtnAccountBusinessField->AccountType, js["AccountType"].get<char>());
            assign(pRtnAccountBusinessField->CurrencyName, js["CurrencyName"].get<std::string>().c_str());
            assign(pRtnAccountBusinessField->CurrencyQuantity, js["CurrencyQuantity"].get<double>());
            assign(pRtnAccountBusinessField->PositionMargin, js["PositionMargin"].get<double>());
            assign(pRtnAccountBusinessField->OrderMargin, js["OrderMargin"].get<double>());
            assign(pRtnAccountBusinessField->PositionBalance, js["PositionBalance"].get<double>());
            assign(pRtnAccountBusinessField->TotalBalance, js["TotalBalance"].get<double>());
            assign(pRtnAccountBusinessField->Available, js["Available"].get<double>());
            assign(pRtnAccountBusinessField->LongAvailable, js["LongAvailable"].get<double>());
            assign(pRtnAccountBusinessField->ShortAvailable, js["ShortAvailable"].get<double>());
            assign(pRtnAccountBusinessField->ActualLongAvail, js["ActualLongAvail"].get<double>());
            assign(pRtnAccountBusinessField->ActualShortAvail, js["ActualShortAvail"].get<double>());
            assign(pRtnAccountBusinessField->Frozen, js["Frozen"].get<double>());
            assign(pRtnAccountBusinessField->Fee, js["Fee"].get<double>());
            assign(pRtnAccountBusinessField->FrozenBuy, js["FrozenBuy"].get<double>());
            assign(pRtnAccountBusinessField->FrozenSell, js["FrozenSell"].get<double>());
            assign(pRtnAccountBusinessField->UpdateTime, js["UpdateTime"].get<std::string>().c_str());
            assign(pRtnAccountBusinessField->CurrencyID, js["CurrencyID"].get<long>());
            assign(pRtnAccountBusinessField->AssetType, js["AssetType"].get<char>());
            assign(pRtnAccountBusinessField->TradeChannel, js["TradeChannel"].get<std::string>().c_str());
            assign(pRtnAccountBusinessField->Borrow, js["Borrow"].get<double>());
            assign(pRtnAccountBusinessField->Lend, js["Lend"].get<double>());
            assign(pRtnAccountBusinessField->DebtOffset, js["DebtOffset"].get<double>());
            assign(pRtnAccountBusinessField->TransferOffset, js["TransferOffset"].get<double>());
        }
        break;
        case UT_FID_ReqManualTransact:
        {
            auto pReqManualTransactField = (CUTReqManualTransactField*)data;
            assign(pReqManualTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pReqManualTransactField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pReqManualTransactField->AccountName, js["AccountName"].get<std::string>().c_str());
            assign(pReqManualTransactField->BusinessName, js["BusinessName"].get<std::string>().c_str());
            assign(pReqManualTransactField->AccountType, js["AccountType"].get<char>());
            assign(pReqManualTransactField->TransactDirection, js["TransactDirection"].get<char>());
            assign(pReqManualTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pReqManualTransactField->Amount, js["Amount"].get<double>());
            assign(pReqManualTransactField->CreateTime, js["CreateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqTransact:
        {
            auto pReqTransactField = (CUTReqTransactField*)data;
            assign(pReqTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pReqTransactField->AccountNameFrom, js["AccountNameFrom"].get<std::string>().c_str());
            assign(pReqTransactField->AccountNameTo, js["AccountNameTo"].get<std::string>().c_str());
            assign(pReqTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pReqTransactField->Direction, js["Direction"].get<char>());
            assign(pReqTransactField->Amount, js["Amount"].get<double>());
            assign(pReqTransactField->TradeChannelFrom, js["TradeChannelFrom"].get<std::string>().c_str());
            assign(pReqTransactField->TradeChannelTo, js["TradeChannelTo"].get<std::string>().c_str());
            assign(pReqTransactField->Type, js["Type"].get<char>());
            assign(pReqTransactField->Address, js["Address"].get<std::string>().c_str());
            assign(pReqTransactField->AddMemo, js["AddMemo"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspTransact:
        {
            auto pRspTransactField = (CUTRspTransactField*)data;
            assign(pRspTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pRspTransactField->AccountNameFrom, js["AccountNameFrom"].get<std::string>().c_str());
            assign(pRspTransactField->AccountNameTo, js["AccountNameTo"].get<std::string>().c_str());
            assign(pRspTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pRspTransactField->Direction, js["Direction"].get<char>());
            assign(pRspTransactField->Amount, js["Amount"].get<double>());
            assign(pRspTransactField->TradeChannelFrom, js["TradeChannelFrom"].get<std::string>().c_str());
            assign(pRspTransactField->TradeChannelTo, js["TradeChannelTo"].get<std::string>().c_str());
            assign(pRspTransactField->Type, js["Type"].get<char>());
            assign(pRspTransactField->Address, js["Address"].get<std::string>().c_str());
            assign(pRspTransactField->AddMemo, js["AddMemo"].get<std::string>().c_str());
            assign(pRspTransactField->ID, js["ID"].get<std::string>().c_str());
            assign(pRspTransactField->CreateTime, js["CreateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnTransact:
        {
            auto pRtnTransactField = (CUTRtnTransactField*)data;
            assign(pRtnTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pRtnTransactField->AccountNameFrom, js["AccountNameFrom"].get<std::string>().c_str());
            assign(pRtnTransactField->AccountNameTo, js["AccountNameTo"].get<std::string>().c_str());
            assign(pRtnTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pRtnTransactField->Direction, js["Direction"].get<char>());
            assign(pRtnTransactField->Amount, js["Amount"].get<double>());
            assign(pRtnTransactField->TradeChannelFrom, js["TradeChannelFrom"].get<std::string>().c_str());
            assign(pRtnTransactField->TradeChannelTo, js["TradeChannelTo"].get<std::string>().c_str());
            assign(pRtnTransactField->Type, js["Type"].get<char>());
            assign(pRtnTransactField->Address, js["Address"].get<std::string>().c_str());
            assign(pRtnTransactField->AddMemo, js["AddMemo"].get<std::string>().c_str());
            assign(pRtnTransactField->ID, js["ID"].get<std::string>().c_str());
            assign(pRtnTransactField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRtnTransactField->TransactStatus, js["TransactStatus"].get<char>());
            assign(pRtnTransactField->Fee, js["Fee"].get<double>());
            assign(pRtnTransactField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
            assign(pRtnTransactField->CreateTime, js["CreateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_ReqQryTransact:
        {
            auto pReqQryTransactField = (CUTReqQryTransactField*)data;
            assign(pReqQryTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pReqQryTransactField->AccountNameFrom, js["AccountNameFrom"].get<std::string>().c_str());
            assign(pReqQryTransactField->AccountNameTo, js["AccountNameTo"].get<std::string>().c_str());
            assign(pReqQryTransactField->Direction, js["Direction"].get<char>());
            assign(pReqQryTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pReqQryTransactField->TradeChannelFrom, js["TradeChannelFrom"].get<std::string>().c_str());
            assign(pReqQryTransactField->TradeChannelTo, js["TradeChannelTo"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RspQryTransact:
        {
            auto pRspQryTransactField = (CUTRspQryTransactField*)data;
            assign(pRspQryTransactField->EventID, js["EventID"].get<std::string>().c_str());
            assign(pRspQryTransactField->AccountNameFrom, js["AccountNameFrom"].get<std::string>().c_str());
            assign(pRspQryTransactField->AccountNameTo, js["AccountNameTo"].get<std::string>().c_str());
            assign(pRspQryTransactField->Currency, js["Currency"].get<std::string>().c_str());
            assign(pRspQryTransactField->Direction, js["Direction"].get<char>());
            assign(pRspQryTransactField->Amount, js["Amount"].get<double>());
            assign(pRspQryTransactField->TradeChannelFrom, js["TradeChannelFrom"].get<std::string>().c_str());
            assign(pRspQryTransactField->TradeChannelTo, js["TradeChannelTo"].get<std::string>().c_str());
            assign(pRspQryTransactField->Type, js["Type"].get<char>());
            assign(pRspQryTransactField->Address, js["Address"].get<std::string>().c_str());
            assign(pRspQryTransactField->AddMemo, js["AddMemo"].get<std::string>().c_str());
            assign(pRspQryTransactField->ID, js["ID"].get<std::string>().c_str());
            assign(pRspQryTransactField->SessionID, js["SessionID"].get<std::string>().c_str());
            assign(pRspQryTransactField->TransactStatus, js["TransactStatus"].get<char>());
            assign(pRspQryTransactField->Fee, js["Fee"].get<double>());
            assign(pRspQryTransactField->FeeCurrency, js["FeeCurrency"].get<std::string>().c_str());
            assign(pRspQryTransactField->CreateTime, js["CreateTime"].get<std::string>().c_str());
        }
        break;
        case UT_FID_RtnDepth:
        {
            auto pRtnDepthField = (CUTRtnDepthField*)data;
            assign(pRtnDepthField->TradingDay, js["TradingDay"].get<std::string>().c_str());
            assign(pRtnDepthField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnDepthField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnDepthField->ExchangeTime, js["ExchangeTime"].get<std::string>().c_str());
            assign(pRtnDepthField->LocalTime, js["LocalTime"].get<std::string>().c_str());
            assign(pRtnDepthField->ArriveTime, js["ArriveTime"].get<std::string>().c_str());
            assign(pRtnDepthField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnDepthField->AskPrice1, js["AskPrice1"].get<double>());
            assign(pRtnDepthField->AskVolume1, js["AskVolume1"].get<double>());
            assign(pRtnDepthField->BidPrice1, js["BidPrice1"].get<double>());
            assign(pRtnDepthField->BidVolume1, js["BidVolume1"].get<double>());
            assign(pRtnDepthField->AskPrice2, js["AskPrice2"].get<double>());
            assign(pRtnDepthField->AskVolume2, js["AskVolume2"].get<double>());
            assign(pRtnDepthField->BidPrice2, js["BidPrice2"].get<double>());
            assign(pRtnDepthField->BidVolume2, js["BidVolume2"].get<double>());
            assign(pRtnDepthField->AskPrice3, js["AskPrice3"].get<double>());
            assign(pRtnDepthField->AskVolume3, js["AskVolume3"].get<double>());
            assign(pRtnDepthField->BidPrice3, js["BidPrice3"].get<double>());
            assign(pRtnDepthField->BidVolume3, js["BidVolume3"].get<double>());
            assign(pRtnDepthField->AskPrice4, js["AskPrice4"].get<double>());
            assign(pRtnDepthField->AskVolume4, js["AskVolume4"].get<double>());
            assign(pRtnDepthField->BidPrice4, js["BidPrice4"].get<double>());
            assign(pRtnDepthField->BidVolume4, js["BidVolume4"].get<double>());
            assign(pRtnDepthField->AskPrice5, js["AskPrice5"].get<double>());
            assign(pRtnDepthField->AskVolume5, js["AskVolume5"].get<double>());
            assign(pRtnDepthField->BidPrice5, js["BidPrice5"].get<double>());
            assign(pRtnDepthField->BidVolume5, js["BidVolume5"].get<double>());
        }
        break;
        case UT_FID_RtnL2Depth:
        {
            auto pRtnL2DepthField = (CUTRtnL2DepthField*)data;
            assign(pRtnL2DepthField->TradingDay, js["TradingDay"].get<std::string>().c_str());
            assign(pRtnL2DepthField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnL2DepthField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnL2DepthField->ExchangeTime, js["ExchangeTime"].get<std::string>().c_str());
            assign(pRtnL2DepthField->LocalTime, js["LocalTime"].get<std::string>().c_str());
            assign(pRtnL2DepthField->ArriveTime, js["ArriveTime"].get<std::string>().c_str());
            assign(pRtnL2DepthField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnL2DepthField->AskPrice1, js["AskPrice1"].get<double>());
            assign(pRtnL2DepthField->AskVolume1, js["AskVolume1"].get<double>());
            assign(pRtnL2DepthField->BidPrice1, js["BidPrice1"].get<double>());
            assign(pRtnL2DepthField->BidVolume1, js["BidVolume1"].get<double>());
            assign(pRtnL2DepthField->AskPrice2, js["AskPrice2"].get<double>());
            assign(pRtnL2DepthField->AskVolume2, js["AskVolume2"].get<double>());
            assign(pRtnL2DepthField->BidPrice2, js["BidPrice2"].get<double>());
            assign(pRtnL2DepthField->BidVolume2, js["BidVolume2"].get<double>());
            assign(pRtnL2DepthField->AskPrice3, js["AskPrice3"].get<double>());
            assign(pRtnL2DepthField->AskVolume3, js["AskVolume3"].get<double>());
            assign(pRtnL2DepthField->BidPrice3, js["BidPrice3"].get<double>());
            assign(pRtnL2DepthField->BidVolume3, js["BidVolume3"].get<double>());
            assign(pRtnL2DepthField->AskPrice4, js["AskPrice4"].get<double>());
            assign(pRtnL2DepthField->AskVolume4, js["AskVolume4"].get<double>());
            assign(pRtnL2DepthField->BidPrice4, js["BidPrice4"].get<double>());
            assign(pRtnL2DepthField->BidVolume4, js["BidVolume4"].get<double>());
            assign(pRtnL2DepthField->AskPrice5, js["AskPrice5"].get<double>());
            assign(pRtnL2DepthField->AskVolume5, js["AskVolume5"].get<double>());
            assign(pRtnL2DepthField->BidPrice5, js["BidPrice5"].get<double>());
            assign(pRtnL2DepthField->BidVolume5, js["BidVolume5"].get<double>());
            assign(pRtnL2DepthField->AskPrice6, js["AskPrice6"].get<double>());
            assign(pRtnL2DepthField->AskVolume6, js["AskVolume6"].get<double>());
            assign(pRtnL2DepthField->BidPrice6, js["BidPrice6"].get<double>());
            assign(pRtnL2DepthField->BidVolume6, js["BidVolume6"].get<double>());
            assign(pRtnL2DepthField->AskPrice7, js["AskPrice7"].get<double>());
            assign(pRtnL2DepthField->AskVolume7, js["AskVolume7"].get<double>());
            assign(pRtnL2DepthField->BidPrice7, js["BidPrice7"].get<double>());
            assign(pRtnL2DepthField->BidVolume7, js["BidVolume7"].get<double>());
            assign(pRtnL2DepthField->AskPrice8, js["AskPrice8"].get<double>());
            assign(pRtnL2DepthField->AskVolume8, js["AskVolume8"].get<double>());
            assign(pRtnL2DepthField->BidPrice8, js["BidPrice8"].get<double>());
            assign(pRtnL2DepthField->BidVolume8, js["BidVolume8"].get<double>());
            assign(pRtnL2DepthField->AskPrice9, js["AskPrice9"].get<double>());
            assign(pRtnL2DepthField->AskVolume9, js["AskVolume9"].get<double>());
            assign(pRtnL2DepthField->BidPrice9, js["BidPrice9"].get<double>());
            assign(pRtnL2DepthField->BidVolume9, js["BidVolume9"].get<double>());
            assign(pRtnL2DepthField->AskPrice10, js["AskPrice10"].get<double>());
            assign(pRtnL2DepthField->AskVolume10, js["AskVolume10"].get<double>());
            assign(pRtnL2DepthField->BidPrice10, js["BidPrice10"].get<double>());
            assign(pRtnL2DepthField->BidVolume10, js["BidVolume10"].get<double>());
        }
        break;
        case UT_FID_RtnL2Trade:
        {
            auto pRtnL2TradeField = (CUTRtnL2TradeField*)data;
            assign(pRtnL2TradeField->ExchangeTime, js["ExchangeTime"].get<std::string>().c_str());
            assign(pRtnL2TradeField->LocalTime, js["LocalTime"].get<std::string>().c_str());
            assign(pRtnL2TradeField->ArriveTime, js["ArriveTime"].get<std::string>().c_str());
            assign(pRtnL2TradeField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnL2TradeField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnL2TradeField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnL2TradeField->LastPrice, js["LastPrice"].get<double>());
            assign(pRtnL2TradeField->Side, js["Side"].get<char>());
            assign(pRtnL2TradeField->CeilingPrice, js["CeilingPrice"].get<double>());
            assign(pRtnL2TradeField->FloorPrice, js["FloorPrice"].get<double>());
            assign(pRtnL2TradeField->BestAsk, js["BestAsk"].get<double>());
            assign(pRtnL2TradeField->BestBid, js["BestBid"].get<double>());
            assign(pRtnL2TradeField->Quantity, js["Quantity"].get<double>());
            assign(pRtnL2TradeField->TotalQuantity, js["TotalQuantity"].get<double>());
        }
        break;
        case UT_FID_RtnL2Order:
        {
            auto pRtnL2OrderField = (CUTRtnL2OrderField*)data;
            assign(pRtnL2OrderField->ExchangeTime, js["ExchangeTime"].get<std::string>().c_str());
            assign(pRtnL2OrderField->LocalTime, js["LocalTime"].get<std::string>().c_str());
            assign(pRtnL2OrderField->ArriveTime, js["ArriveTime"].get<std::string>().c_str());
            assign(pRtnL2OrderField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnL2OrderField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnL2OrderField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnL2OrderField->Price, js["Price"].get<double>());
            assign(pRtnL2OrderField->Volume, js["Volume"].get<double>());
        }
        break;
        case UT_FID_RtnL2Index:
        {
            auto pRtnL2IndexField = (CUTRtnL2IndexField*)data;
            assign(pRtnL2IndexField->ExchangeTime, js["ExchangeTime"].get<std::string>().c_str());
            assign(pRtnL2IndexField->LocalTime, js["LocalTime"].get<std::string>().c_str());
            assign(pRtnL2IndexField->ArriveTime, js["ArriveTime"].get<std::string>().c_str());
            assign(pRtnL2IndexField->PlatformTime, js["PlatformTime"].get<std::string>().c_str());
            assign(pRtnL2IndexField->ExchangeID, js["ExchangeID"].get<std::string>().c_str());
            assign(pRtnL2IndexField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnL2IndexField->PreCloseIndex, js["PreCloseIndex"].get<double>());
            assign(pRtnL2IndexField->OpenIndex, js["OpenIndex"].get<double>());
            assign(pRtnL2IndexField->CloseIndex, js["CloseIndex"].get<double>());
            assign(pRtnL2IndexField->HighIndex, js["HighIndex"].get<double>());
            assign(pRtnL2IndexField->LowIndex, js["LowIndex"].get<double>());
            assign(pRtnL2IndexField->LastIndex, js["LastIndex"].get<double>());
            assign(pRtnL2IndexField->TurnOver, js["TurnOver"].get<double>());
            assign(pRtnL2IndexField->TotalVolume, js["TotalVolume"].get<double>());
        }
        break;
        case UT_FID_RtnBarMarketData:
        {
            auto pRtnBarMarketDataField = (CUTRtnBarMarketDataField*)data;
            assign(pRtnBarMarketDataField->TradingDay, js["TradingDay"].get<std::string>().c_str());
            assign(pRtnBarMarketDataField->InstrumentID, js["InstrumentID"].get<std::string>().c_str());
            assign(pRtnBarMarketDataField->CeilingPrice, js["CeilingPrice"].get<double>());
            assign(pRtnBarMarketDataField->FloorPrice, js["FloorPrice"].get<double>());
            assign(pRtnBarMarketDataField->StartUpdateTime, js["StartUpdateTime"].get<std::string>().c_str());
            assign(pRtnBarMarketDataField->EndUpdateTime, js["EndUpdateTime"].get<std::string>().c_str());
            assign(pRtnBarMarketDataField->Open, js["Open"].get<double>());
            assign(pRtnBarMarketDataField->Close, js["Close"].get<double>());
            assign(pRtnBarMarketDataField->Low, js["Low"].get<double>());
            assign(pRtnBarMarketDataField->High, js["High"].get<double>());
            assign(pRtnBarMarketDataField->Volume, js["Volume"].get<double>());
            assign(pRtnBarMarketDataField->StartVolume, js["StartVolume"].get<double>());
        }
        break;
        default:
        {
            std::stringstream sstream;
            sstream << "Unexcept Type: " << msg_type;
        }
    }
}


PANDORA_NAMESPACE_END



