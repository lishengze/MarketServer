#pragma once
#include "../pandora_declare.h"
#include "digital_position_handler.h"
#include "quark/cxx/customDataType.h"
#include "../messager/messager.h"
#include "quark/cxx/Utils.h"
#include "../messager/ut_log.h"

PANDORA_NAMESPACE_START

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the position Handler Base
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// as a base class that all need to handle position behaviour
class PositionHandler : public IMessager, public IRedisPublisher
{
public:
    PositionHandler(const string& account_name, const char& account_type, IAssetFactory* asset_factory) : account_name_{account_name}, account_type_{account_type}, asset_factory_{asset_factory}
    {}

    virtual ~PositionHandler() = default;

    // retrieve the account name
    string account_name() { return account_name_; }

    // req_insert_order
    // @return true if we can creat order, or return false
    // @pReqCreateOrderField req create order orbject
    virtual bool req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField)
    {
        IPositionModulePtr pPositionModule = get_module(pReqCreateOrderField->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->req_insert_order(pReqCreateOrderField);
        }
        return false;
    }

    // rollback frozon order, we have several riskctrl condition when we create order, if one of them failed, roolback all accounts frozen before
    // @return bool
    // @pReqCreateOrderField req create order
    virtual bool rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField)
    {
        IPositionModulePtr pPositionModule = get_module(pReqCreateOrderField->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->rollback_order_frozen(pReqCreateOrderField);
        }
        return false;
    }

    // on_rsp_insert_order
    // @return none
    // @pRspCreateOrderField response of create order
    virtual bool on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo)
    {
        IPositionModulePtr pPositionModule = get_module(pRspCreateOrderField->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->on_rsp_insert_order(pRspCreateOrderField, pRspInfo);
        }
        return false;
    }

    // on_rsp_cancel_order
    // @return none
    // @pRspCancelOrderField response of cancel order
    virtual bool on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo)
    {
        IPositionModulePtr pPositionModule = get_module(pRspCancelOrderField->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->on_rsp_cancel_order(pRspCancelOrderField, pRspInfo);
        }
        return false;
    }

    // on_rtn_order
    // @return none
    // @pRtnOrderField response of create order
    virtual bool on_rtn_order(const CUTRtnOrderField* pRtnOrderField)
    {
        IPositionModulePtr pPositionModule = get_module(pRtnOrderField->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->on_rtn_order(pRtnOrderField);
        }
        return false;
    }
    
    // on_rtn_trade
    // @return bool handle result
    // @pRtnTrade return trade info
    virtual bool on_rtn_trade(const CUTRtnTradeField* pRtnTrade)
    {
        IPositionModulePtr pPositionModule = get_module(pRtnTrade->AssetType);
        if (pPositionModule)
        {
            return pPositionModule->on_rtn_trade(pRtnTrade);
        }
        return false;
    }

    // get the position module
    IPositionModulePtr get_module(const CAssetTypeType& type)
    {
        switch(type)
        {
            case AST_DigitalFuture:
            {
                if (!digital_future_account_)
                {
                    digital_future_account_ = DigitalFutureAccountPtr{new DigitalFutureAccount{account_name_, account_type_, asset_factory_}};

                    digital_future_account_->set_logger(logger_);
                    digital_future_account_->set_publisher(redis_publisher_);
                    digital_future_account_->set_talker(talker_);
                    LOG_DEBUG("[CreateDigitalFuture] [AccountName=" << account_name_ << "] [AccountType=" << account_type_ << "]");
                }
                return digital_future_account_;
            }
            case AST_DigitalSpot:
            {
                if (!digital_spot_account_)
                {
                    digital_spot_account_ = DigitalSpotAccountPtr{new DigitalSpotAccount{account_name_, account_type_, asset_factory_}};

                    digital_spot_account_->set_logger(logger_);
                    digital_spot_account_->set_publisher(redis_publisher_);
                    digital_spot_account_->set_talker(talker_);
                    LOG_DEBUG("[CreateSpotFuture] [AccountName=" << account_name_ << "] [AccountType=" << account_type_ << "]");
                }
                return digital_spot_account_;
            }
            default:
            {
                LOG_WARN("Failed to get account module when get_module, asset_type: " + getAssetTypeString(type));
                return nullptr;
            }
        }
    }

private:
    // account name
    string account_name_;
    // account type
    char account_type_;
    // asset_factory
    IAssetFactory* asset_factory_;

    // different kind of position calculation
    // the digital future account
    DigitalFutureAccountPtr digital_future_account_{nullptr};
    // the digital spot account
    DigitalSpotAccountPtr   digital_spot_account_{nullptr};
};
DECLARE_PTR(PositionHandler);

PANDORA_NAMESPACE_END