#include "../pandora_declare.h"
#include "quark/cxx/ut/UtData.h"
#include "instrument_property.h"
#include "asset_cell.h"
#include "asset_factory.h"
#include "../messager/messager.h"
#include "position_module.h"

PANDORA_NAMESPACE_START

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position spot account riskcontrol, to solve the spot commodity's riskcontrol
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DigitalSpotAccount : public IPositionModule
{
    // define currency type, currency pair type, symbol currency pair type
    using CurrencyPairType = std::pair<ICurrencyCellPtr, ICurrencyCellPtr>;
    using SymbolBalanceType = std::unordered_map<string, CurrencyPairType>;
public:
    DigitalSpotAccount(const string& account_name, const char& account_type, IAssetFactory* asset_factory);
    virtual ~DigitalSpotAccount() {}
    // create balance virtual method
    // virtual ICurrencyCellPtr create_balance(const string& currency_name) override;

    // req_insert_order
    // @return return true if we can creat order, or return false
    // @pReqCreateOrderField req create order orbject
    virtual bool req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField) override;

    // rollback frozon order, we have several riskctrl condition when we create order, if one of them failed, roolback all accounts frozen before.
    // @return bool
    // @pReqCreateOrderField req create order
    virtual bool rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField) override;

    // on_rsp_insert_order
    // @return none
    // @pRspCreateOrderField response of create order
    virtual bool on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo) override;

    // on_rsp_cancel_order
    // @return none
    // @pRspCancelOrderField response of cancel order
    virtual bool on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo) override;

    // on_rtn_order
    // @return none
    // @pRtnOrderField response of create order
    virtual bool on_rtn_order(const CUTRtnOrderField* pRtnOrderField) override;
    
    // on_rtn_trade
    // @return bool handle result
    // @pRtnTrade return trade info
    virtual bool on_rtn_trade(const CUTRtnTradeField* pRtnTrade) override;

private:
    double get_fee(InstrumentPropertyPtr inst_property, const CUTRtnTradeField* pRtnTrade, const double& match_price);
    // get the symbol balance object pair
    CurrencyPairType get_symbol_balance(const std::string& symbol, const std::string& exchangeid);

    // rollback_order
    // @return rollback order result
    // @pOrderField order construct object
    // @volume_remain volume remain
    template<typename T>
    bool rollback_order(const T* pOrderField, const double& volume_remain)
    {
        // read the direction 
        int direction = pOrderField->Direction==BS_Long ? 1:-1;

        CurrencyPairType currency_pair = get_symbol_balance(pOrderField->InstrumentID, pOrderField->ExchangeID);
        currency_pair.first->unfrozen(volume_remain*direction);
        currency_pair.second->unfrozen(pOrderField->Price*volume_remain*direction*-1);
        asset_factory_->currency_changed_notify(currency_pair.first);
        asset_factory_->currency_changed_notify(currency_pair.second);

        return true;
    }
    // symbol -> symbol balance
    SymbolBalanceType SymbolBalance;
};
DECLARE_PTR(DigitalSpotAccount);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// position future account riskcontrol, to solve the future commodity's riskcontrol
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DigitalFutureAccount : public IPositionModule
{
public:
    DigitalFutureAccount(const string& account_name, const char& account_type, IAssetFactory* asset_factory);
    virtual ~DigitalFutureAccount() {}

    // req_insert_order
    // @return return true if we can creat order, or return false
    // @pReqCreateOrderField req create order orbject
    virtual bool req_insert_order(const CUTReqCreateOrderField* pReqCreateOrderField) override;

    // rollback frozon order, we have several riskctrl condition when we create order, if one of them failed, roolback all accounts frozen before.
    // @return bool
    // @pReqCreateOrderField req create order
    virtual bool rollback_order_frozen(const CUTReqCreateOrderField* pReqCreateOrderField) override;

    // on_rsp_insert_order
    // @return none
    // @pRspCreateOrderField response of create order
    virtual bool on_rsp_insert_order(const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfo) override;

    // on_rsp_cancel_order
    // @return none
    // @pRspCancelOrderField response of cancel order
    virtual bool on_rsp_cancel_order(const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfo) override;

    // on_rtn_order
    // @return none
    // @pRtnOrderField response of create order
    virtual bool on_rtn_order(const CUTRtnOrderField* pRtnOrderField) override;
    
    // on_rtn_trade
    // @return bool handle result
    // @pRtnTrade return trade info
    virtual bool on_rtn_trade(const CUTRtnTradeField* pRtnTrade) override;

private:
    // convert price
    double convert_price(InstrumentPropertyPtr inst_property, const double& price);
    // set margin
    void set_margin(InstrumentPropertyPtr, const CUTRtnTradeField*, const double&, const double&, double&, double&);
    // get trade fee
    double get_fee(InstrumentPropertyPtr inst_property, const CUTRtnTradeField* pRtnTrade, const double& match_price);
    // rollback_order
    // @return rollback order result
    // @pOrderField order construct object
    // @volume_remain volume_remain
    template<typename T>
    bool rollback_order(const T* pOrderField, const double& volume_remain)
    {
        //  retrieve the instrument property
        InstrumentPropertyPtr inst_property = asset_factory_->retrieve_instrument_property(pOrderField->ExchangeID, pOrderField->InstrumentID);
        double margin_rate{1};
        if (inst_property)
        {
            margin_rate=inst_property->MarginRate;
        }
        else
        {
            return false;
        }
        // read the direction
        int direction = pOrderField->Direction==BS_Long ? 1:-1;
        // retrieve the position and currency balance
        IPositionCellPtr position = asset_factory_->retrieve_position(inst_property->BaseCurrency, pOrderField->InstrumentID, asset_type_, pOrderField->ExchangeID, pOrderField->Direction, inst_property->ClassicSettle);
        ICurrencyCellPtr currency_balance = asset_factory_->retrieve_currency(inst_property->BaseCurrency, asset_type_, pOrderField->ExchangeID);
        // calculate the quota
        double position_quota{volume_remain*direction};
        double margin_quota{0.0};
        double order_price{convert_price(inst_property, pOrderField->Price)};
        if (margin_rate < 1)
            margin_quota = order_price*volume_remain*margin_rate*inst_property->Multiple;
        else
            margin_quota = volume_remain*margin_rate;
        // unfrozen quota
        position->unfrozen(position_quota, margin_quota);
        currency_balance->unfrozen_margin(margin_quota);
        // reset recent update position & currency
        asset_factory_->currency_changed_notify(currency_balance);

        return true;
    }
};
DECLARE_PTR(DigitalFutureAccount);

PANDORA_NAMESPACE_END