#pragma once
//
// Created by daniel on 9/24/19.
//
#include "../pandora_declare.h"
#include "../messager/messager.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// digital account riskcontrol baseclass, to solve some common issue
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PANDORA_NAMESPACE_START
class IPositionModule :  public IMessager, public IRedisPublisher
{
public:
    // digital module, base class for configuration
    IPositionModule(const string& account_name, const char& account_type, const char& asset_type, IAssetFactory* asset_factory) : account_name_{account_name}, account_type_{account_type}, asset_type_{asset_type}, asset_factory_{asset_factory}
    {}
    virtual ~IPositionModule() {}
    // req_insert_order
    // @return return true if we can creat order, or return false
    // @pReqCreateOrderField req create order orbject
    virtual bool req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField) = 0;

    // rollback frozon order, we have several riskctrl condition when we create order, if one of them failed, roolback all accounts frozen before.
    // @return bool
    // @pReqCreateOrderField req create order
    virtual bool rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField) = 0;

    // on_rsp_insert_order
    // @return none
    // @pRspCreateOrderField response of create order
    virtual bool on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo) = 0;

    // on_rsp_cancel_order
    // @return none
    // @pRspCancelOrderField response of cancel order
    virtual bool on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo) = 0;

    // on_rtn_order
    // @return none
    // @pRtnOrderField response of create order
    virtual bool on_rtn_order(const CUTRtnOrderField* pRtnOrderField) = 0;

    // on_rtn_trade
    // @return bool handle result
    // @pRtnTrade return trade info
    virtual bool on_rtn_trade(const CUTRtnTradeField* pRtnTrade) = 0;


protected:
    // account name
    string account_name_;
    // account type
    char account_type_;
    // asset type
    char asset_type_{'U'};
    // 资产工厂访问接口
    IAssetFactory* asset_factory_;
};
DECLARE_PTR(IPositionModule);

PANDORA_NAMESPACE_END