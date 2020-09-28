#pragma once
#include "../pandora_declare.h"
#include "asset_cell.h"
#include "instrument_property.h"
#include "quark/cxx/customDataType.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "quark/cxx/error/error_define.h"

PANDORA_NAMESPACE_START

class IAssetFactory
{
public:
    IAssetFactory(const std::string& account_name, const CAccountTypeType& account_type) : account_name_{account_name}, account_type_{account_type} {}
    virtual ~IAssetFactory() { }

    // 从资产工厂里面获取币种信息
    // @currency_name 货币名称，例如 ETH BTC etc
    // @asset_type 资产类型，用来做什么的资产，如数字货币期货、数字货币现货、常规期货
    // @exchangeid 建立在哪个交易所的持仓(一般跟随报单来的，虽然逻辑账户没有具体交易所，但是报单是有的)
    // @auto_create 当账户不存在时是否需要自动创建
    ICurrencyCellPtr retrieve_currency(const string& currency_name, const CAssetTypeType& asset_type, const std::string& exchangeid="",  bool auto_create=true)
    {
        std::string currency_key{currency_name+std::to_string(asset_type)+exchangeid};
        auto iter_find_currency = currency_balance_.find(currency_key);
        if (iter_find_currency!=currency_balance_.end())
        {
            return iter_find_currency->second;
        }
        else
        {
            if (auto_create)
            {
                // if no currency balance now, force create it
                ICurrencyCellPtr currency_cell{create_currency(currency_name, asset_type, exchangeid)};
                if (!exchangeid.empty())  currency_cell->set_exchange_id(exchangeid);
                currency_balance_.emplace(currency_key, currency_cell);
                return currency_cell;
            }
            else
            {
                return nullptr;
            }
        }
    }

    // 从资产工厂里面获取持仓信息
    // @currency_name 建立在哪个货币上的持仓
    // @asset_type 建立在哪个货币资产类型上的持仓, eg 数字货币期货、数字货币现货、常规期货
    // @exchangeid 建立在哪个交易所的持仓，当然也会有可能为空，比如说自撮合持仓
    // @instrument 持仓合约名称
    // @direction 持仓方向，比如多、空、净
    // @classic_settle 这个为次合约的结算方式
    IPositionCellPtr retrieve_position(const string& currency_name, const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid, const CPosiDirectionType& direction, bool classic_settle)
    {
        // retrieve currency balance
        ICurrencyCellPtr currency_cell = retrieve_currency(currency_name, asset_type);
        // retrieve the positon from instrumentid
        IPositionCellPtr position = currency_cell->retrieve_position(instrumentid);
        if (!position)
        {
            IPositionCellPtr position_cell{create_position(instrumentid, asset_type, exchangeid, direction)};
            position->set_settle(classic_settle);
            currency_cell->bind_position(position, instrumentid);
        }
        return position;
    }

    // 从资产工厂里面获取持仓信息
    // @asset_type 建立在哪个货币资产类型上的持仓, eg 数字货币期货、数字货币现货、常规期货
    // @exchangeid 建立在哪个交易所的持仓，当然也会有可能为空，比如说自撮合持仓
    // @instrument 持仓合约名称
    // @direction 持仓方向，比如多、空、净
    IPositionCellPtr retrieve_position(const string& instrumentid, const CAssetTypeType& asset_type, const string& exchangeid,  const CPosiDirectionType& direction)
    {
        // retrieve the inst property
        InstrumentPropertyPtr inst_property = retrieve_instrument_property(exchangeid, instrumentid);
        if (!inst_property) return nullptr;
        // retrieve currency balance
        ICurrencyCellPtr currency_cell = retrieve_currency(inst_property->BaseCurrency, asset_type, exchangeid);
        // retrieve the position from instrument id
        IPositionCellPtr position = currency_cell->retrieve_position(instrumentid);
        if (position)
        {
            IPositionCellPtr position_cell{create_position(instrumentid, asset_type, exchangeid, direction)};
            position->set_settle(inst_property->ClassicSettle);
            currency_cell->bind_position(position, instrumentid);
        }
        return position;
    }

    // 从资产工厂里面获取合约相关信息
    // @exchangeid 交易所编号
    // @instrumentid 合约编号
    InstrumentPropertyPtr retrieve_instrument_property(const std::string& exchangeid, const std::string& instrumentid)
    {
        auto iter_find_inst = instrument_property_buffer_.find(exchangeid+instrumentid);
        if (iter_find_inst!=instrument_property_buffer_.end())
        {
            return iter_find_inst->second;
        }
        else
        {
            if (instrument_properties_)
            {
                InstrumentPropertyPtr inst_property = instrument_properties_->get_instrument_property(exchangeid, instrumentid);
                if (inst_property) 
                {
                    instrument_property_buffer_.emplace(exchangeid+instrumentid, inst_property);
                    return inst_property;
                }
                else
                {
                    std::string errormsg = std::to_string(ERROR_INSTRUMENTID_CONFIG) + " Failed Get Instrument Property [AccountName=" + account_name_ + "] [InstrumentID=" + instrumentid + "] [ExchangeID=" + exchangeid + "] [AccountType=" + std::to_string(account_type_) + "]";
                    log(LL_Warning, errormsg);
                    dingtalk(account_name_, LL_Warning, ERROR_INSTRUMENTID_CONFIG, errormsg);
                }
            }
            else
            {
                std::string errormsg = std::to_string(ERROR_INSTRUMENTID_CONFIG) + "Failed Get Instrument Properties [AccountName=" + account_name_ + "] [InstrumentID=" + instrumentid + "] [ExchangeID=" + exchangeid + "] [AccountType=" + std::to_string(account_type_) + "]";
                log(LL_Warning, errormsg);
                dingtalk(account_name_, LL_Warning, ERROR_INSTRUMENTID_CONFIG, errormsg);
            }
        }
        return nullptr;
    }

    // 初始化持币信息
    void init_currency(const CUTRtnAccountField& account)
    {
        log(LL_Debug, "[InitCurrency] [AccountName=" + account_name_ + "] [AssetType=" + getAccountTypeString(account_type_) + "] [Currency=" + account.CurrencyName + "]");
        log(LL_Debug, convertUTData(&account, UT_FID_RtnAccount));
        ICurrencyCellPtr currency_balance = retrieve_currency(account.CurrencyName, account.AssetType, account.ExchangeID);
        currency_balance->init_currency(account);
    }

    // 初始化持仓信息
    void init_position(CUTRtnPositionField& position)
    {
        log(LL_Debug, "[InitPosition] [AccountName=" + account_name_ + "] [AssetType=" + getAssetTypeString(account_type_) + "] [ExchangeID=" + position.ExchangeID + "] [Instrument=" + position.InstrumentID + "] [PosiDirection=" + getPosiDirectionString(position.PosiDirection) + "]");
        log(LL_Debug, convertUTData(&position, UT_FID_RtnPosition));
        // get the instrument property accord to exchangeid and instrumentid
        InstrumentPropertyPtr inst_property = retrieve_instrument_property(position.ExchangeID, position.InstrumentID);
        if (!inst_property) return;
        IPositionCellPtr position_cell = retrieve_position(inst_property->BaseCurrency, position.InstrumentID, position.AssetType, position.ExchangeID,  position.PosiDirection, inst_property->ClassicSettle);

        if (!inst_property->ClassicSettle)
             assign(position.Price, 1.0/position.Price);
        position_cell->init_position(position);
    }

    // 账户名称 
    const std::string& account_name() const { return account_name_; };
    // 账户类型
    const CAccountTypeType& account_type() const { return account_type_; }
    // 交易所编号
    const std::string& exchange_id() const { return exchange_id_; }
    // 设定交易所
    void set_exchange_id(const std::string& exchangeid) { exchange_id_ = exchangeid; }

    // 创建指定币种
    virtual ICurrencyCellPtr create_currency(const std::string& currency_name, const CAssetTypeType& asset_type, const std::string& exchangeid) = 0;
    // 创建指定持仓
    virtual IPositionCellPtr create_position(const string& instrumentid, const CAssetTypeType& asset_type, const std::string& exchangeid,  const CPosiDirectionType& direction) = 0;
    // 发布日志信息
    virtual void log(CLogLevelType, const string&) {}
    // 发布钉钉信息
    virtual void dingtalk(const string& title, CLogLevelType level, int errorid, const string& errormsg) {}
    // 通知持币产生了变更
    virtual void currency_changed_notify(ICurrencyCellPtr) {}
    // 通知持仓产生了变化
    virtual void position_changed_notify(IPositionCellPtr) {}

protected:
    // currency balance， 注意持仓信息挂载在币种上
    std::unordered_map<string, utrade::pandora::ICurrencyCellPtr> currency_balance_;
    // instrument properties
    InstrumentPropertiesPtr instrument_properties_;
    // instrument properties buffer
    std::unordered_map<string, InstrumentPropertyPtr> instrument_property_buffer_;
    // 以下为每个账户必须具有的属性（也许对于某些复合账户而言不存在交易所名称）
    std::string account_name_{""};      // 账户名称
    CAccountTypeType account_type_;     // 账户类型
    std::string exchange_id_{""};       // 交易所名称
};
DECLARE_PTR(IAssetFactory);

PANDORA_NAMESPACE_END
