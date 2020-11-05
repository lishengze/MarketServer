#pragma once
#include "../pandora_declare.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include "quark/cxx/customDataType.h"
#include "quark/cxx/stg/StgData.h"
#include "assign.h"
#include "../util/time_util.h"
#include "quark/cxx/ut/UtData.h"
#include "quark/cxx/ut/UtCopyEntity.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "../messager/redis_publisher.h"
#include "../messager/ut_log.h"
#include "quark/cxx/ut/UtFuncData.h"
#include <boost/make_shared.hpp>
#include <functional>
#include <iostream>


PANDORA_NAMESPACE_START

template<class T>
struct my_less
{
    bool operator()(const char *p1,const char *p2) const
    {
        return strcmp(p1,p2)<0;
    }
    bool operator()(const std::string &p1,const std::string &p2) const
    {
        return p1<p2;
    }
    bool operator()(const std::string &p1,const char *p2) const
    {
        return strcmp(p1.c_str(),p2)<0;
    }
    bool operator()(const char *p1,const std::string &p2) const
    {
        return strcmp(p1,p2.c_str())<0;
    }
    bool operator()(const char*& p1, const char*& p2)
    {
        return strcmp(p1, p2) < 0;
    }
    //对于数值类型的数据，系统中除了字符串数组，就是数值类型的
//    bool operator()(T _Left, T _Right) const
//    {
//        return std::less<T>()(_Left,_Right);
//    }
};


// define index
class Tag_OrderSysIDIndex{};
class Tag_OrderLocalIDIndex{};
class Tag_TradeIDIndex{};
class Tag_TradeOrderSysIDIndex{};
#define SequenceIndex 2

// define order container
typedef boost::multi_index::multi_index_container<
        CUTRtnOrderFieldPtr,
        boost::multi_index::indexed_by <
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<Tag_OrderSysIDIndex>,
                        boost::multi_index::member<
                                CUTRtnOrderField, COrderSysIDType, &CUTRtnOrderField::OrderSysID
                        >,
                        my_less<COrderSysIDType>
                        //std::less<COrderSysIDType>
                >,
                boost::multi_index::ordered_unique<
                        boost::multi_index::tag<Tag_OrderLocalIDIndex>,
                        boost::multi_index::member<
                                CUTRtnOrderField, COrderLocalIDType, &CUTRtnOrderField::OrderLocalID
                        >,
                        my_less<COrderLocalIDType>
                >,
                boost::multi_index::sequenced<>
        >
> Order_Container;

// define trade container
typedef boost::multi_index::multi_index_container<
        CUTRtnTradeFieldPtr,
        boost::multi_index::indexed_by <
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<Tag_TradeIDIndex>,
                        boost::multi_index::member<
                                CUTRtnTradeField, CTradeIDType, &CUTRtnTradeField::TradeID
                        >,
                        my_less<CTradeIDType>
                >,
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<Tag_TradeOrderSysIDIndex>,
                        boost::multi_index::member<
                                CUTRtnTradeField, COrderSysIDType, &CUTRtnTradeField::OrderSysID
                        >,
                        my_less<COrderSysIDType>
                >,
                boost::multi_index::sequenced<>
        >
> Trade_Container;

// erase complete order
#define ERASE_COMPLETE_ORDER(field)  \
if (OST_Rejected==field->OrderStatus || OST_Killed==field->OrderStatus || OST_Filled==field->OrderStatus) \
{   \
    auto& trade_idx = trades_.get<Tag_TradeOrderSysIDIndex>();  \
    auto iter_trade = trade_idx.find((*iter_order)->OrderSysID);   \
    while (iter_trade != trade_idx.end()) iter_trade = trade_idx.erase(iter_trade); \
    std::cout<<(*iter_order)->OrderSysID << "erase"<<std::endl;\
    order_idx.erase(iter_order);    \
}
// 报单管理器
class OrderManager
{
public:
    OrderManager() {}
    virtual ~OrderManager() {}
    // bind the publisher
    void bind_publisher(RedisPublisherPtr publisher) { publisher_=publisher; }
    // bind the logger
    void bind_logger(UTLogPtr logger) { logger_=logger; }

    // begin the insert order
    void ReqCreateOrder(const CUTReqCreateOrderField* pReqCreateOrderField)
    {
        CUTRtnOrderFieldPtr pNewOrder{new CUTRtnOrderField{}};
        UTCopyRtnOrderEntity(pNewOrder.get(), pReqCreateOrderField);
        orders_.insert(pNewOrder);
    }

    // process package
    bool process_package(PackagePtr package)
    {
        switch (package->Tid())
        {
            case UT_TID_ReqCreateOrder:
            {
                auto pUTReqCreateOrderField = GET_FIELD(package, CUTReqCreateOrderField);
                CUTRtnOrderFieldPtr pNewOrder{new CUTRtnOrderField{}};
                UTCopyRtnOrderEntity(pNewOrder.get(), pUTReqCreateOrderField);
                orders_.insert(pNewOrder);

                if (publisher_) publisher_->publish_package(package);
                if (logger_) UT_LOG_INFO(logger_, "[OrderManager::ReqCreateOrder] [OrderLocalID=" << pUTReqCreateOrderField->OrderLocalID << "]");
                return true;
            }
                break;
            case UT_TID_RspCreateOrder:
            {
                auto pUTRspCreateOrderField = GET_NON_CONST_FIELD(package, CUTRspCreateOrderField);
                auto& order_idx = orders_.get<Tag_OrderLocalIDIndex>();
                auto iter_order = order_idx.find(pUTRspCreateOrderField->OrderLocalID);
                if (iter_order != order_idx.end())
                {
                    assign(pUTRspCreateOrderField->OrderForeID, (*iter_order)->OrderForeID);
                    assign(pUTRspCreateOrderField->AssetType, (*iter_order)->AssetType);

                    assign((*iter_order)->OrderStatus, pUTRspCreateOrderField->OrderStatus);
                    if (strcmp(pUTRspCreateOrderField->OrderSysID, ""))
                    {
                        assign((*iter_order)->OrderSysID, pUTRspCreateOrderField->OrderSysID);
                        std::cout << "Rsp" << (*iter_order)->OrderSysID << std::endl;
                    }
                    ERASE_COMPLETE_ORDER(pUTRspCreateOrderField);

                    if (publisher_) publisher_->publish_package(package);
                    if (logger_) UT_LOG_INFO(logger_, "[OrderManager::RspCreateOrder] [OrderLocalID=" << pUTRspCreateOrderField->OrderLocalID << "] [Status=" << pUTRspCreateOrderField->OrderStatus << "]");
                    return true;
                }
                else
                {
                    if (logger_) UT_LOG_WARNING(logger_, "[OrderManager::RspCreateOrder] NO ORDER RECORD [OrderLocalID=" << pUTRspCreateOrderField->OrderLocalID << "] [Status=" << pUTRspCreateOrderField->OrderStatus << "]");
                    return false;
                }
            }
                break;
            case UT_TID_RspCancelOrder:
            {
                auto pUTRspCancelOrderField = GET_NON_CONST_FIELD(package, CUTRspCancelOrderField);
                auto& order_idx = orders_.get<Tag_OrderLocalIDIndex>();
                auto iter_order = order_idx.find(pUTRspCancelOrderField->OrderLocalID);
                if (iter_order != order_idx.end())
                {
                    assign(pUTRspCancelOrderField->OrderForeID, (*iter_order)->OrderForeID);
                    assign(pUTRspCancelOrderField->Price, (*iter_order)->Price);
                    assign(pUTRspCancelOrderField->Volume, (*iter_order)->Volume);
                    assign(pUTRspCancelOrderField->AssetType, (*iter_order)->AssetType);
                    assign((*iter_order)->OrderStatus, pUTRspCancelOrderField->OrderStatus);
                    ERASE_COMPLETE_ORDER(pUTRspCancelOrderField);

                    if (publisher_) publisher_->publish_package(package);
                    if (logger_) UT_LOG_INFO(logger_, "[OrderManager::RspCancelOrder] [OrderSysID=" << pUTRspCancelOrderField->OrderSysID << "] [Status=" << pUTRspCancelOrderField->OrderStatus << "]");
                    return true;
                }
                else
                {
                    if (logger_) UT_LOG_WARNING(logger_, "[OrderManager::RspCancelOrder] NO ORDER RECORD [OrderSysID=" << pUTRspCancelOrderField->OrderSysID << "] [Status=" << pUTRspCancelOrderField->OrderStatus << "]");
                    return false;
                }
            }
                break;
            case UT_TID_RtnOrder:
            {
                auto pUTRtnOrderField = GET_NON_CONST_FIELD(package, CUTRtnOrderField);
                auto& order_idx = orders_.get<Tag_OrderLocalIDIndex>();
                auto iter_order = order_idx.find(pUTRtnOrderField->OrderLocalID);
                if (iter_order != order_idx.end())
                {
                    CUTRtnOrderFieldPtr rtnOrder = *iter_order;

                    assign(pUTRtnOrderField->OrderForeID, (*iter_order)->OrderForeID);
                    assign(pUTRtnOrderField->Price, (*iter_order)->Price);
                    assign(pUTRtnOrderField->Volume, (*iter_order)->Volume);
                    assign(pUTRtnOrderField->LandTime, (*iter_order)->LandTime);
                    assign(pUTRtnOrderField->SendTime, (*iter_order)->SendTime);
                    assign(pUTRtnOrderField->AssetType, (*iter_order)->AssetType);
                    assign(pUTRtnOrderField->SessionID, (*iter_order)->SessionID);
                    assign(pUTRtnOrderField->TradeChannel, (*iter_order)->TradeChannel);

                    assign(rtnOrder->OrderStatus, pUTRtnOrderField->OrderStatus);
                    assign(rtnOrder->TradeVolume, pUTRtnOrderField->TradeVolume);

                    //assign((*iter_order)->OrderStatus, pUTRtnOrderField->OrderStatus);
                    //assign((*iter_order)->TradeVolume, pUTRtnOrderField->TradeVolume);
                    if (strcmp(pUTRtnOrderField->OrderSysID, ""))
                    {
                        //assign(rtnOrder->OrderSysID, pUTRtnOrderField->OrderSysID);
                        //assign((*iter_order)->OrderSysID, pUTRtnOrderField->OrderSysID);

                        //order_idx.replace(iter_order, rtnOrder);
                        order_idx.modify(iter_order, [&](CUTRtnOrderFieldPtr ptr){
                            assign(ptr->OrderSysID, pUTRtnOrderField->OrderSysID);
                            assign(ptr->OrderStatus, pUTRtnOrderField->OrderStatus);
                            assign(ptr->TradeVolume, pUTRtnOrderField->TradeVolume);
                        });
                        std::cout<<"RtnOrder" << rtnOrder->OrderSysID<<std::endl;
                    }

                    ERASE_COMPLETE_ORDER(pUTRtnOrderField);

                    if (publisher_) publisher_->publish_package(package);
                    if (logger_) UT_LOG_INFO(logger_, "[OrderManager::RtnOrder] [OrderSysID=" << pUTRtnOrderField->OrderSysID << "] [Status=" << pUTRtnOrderField->OrderStatus << "]");
                    return true;
                }
                else
                {
                    if (logger_) UT_LOG_WARNING(logger_, "[OrderManager::RtnOrder] NO ORDER RECORD [OrderSysID=" << pUTRtnOrderField->OrderSysID << "] [Status=" << pUTRtnOrderField->OrderStatus << "]");
                    return false;
                }
            }
                break;
            case UT_TID_RtnTrade:
            {
                auto pUTRtnTradeField = GET_NON_CONST_FIELD(package, CUTRtnTradeField);
                auto& order_idx = orders_.get<Tag_OrderSysIDIndex>();
                auto iter_order = order_idx.find(pUTRtnTradeField->OrderSysID);
                if (iter_order != order_idx.end())
                {
                    assign(pUTRtnTradeField->Price, (*iter_order)->Price);
                    assign(pUTRtnTradeField->AssetType, (*iter_order)->AssetType);
                    assign(pUTRtnTradeField->OrderMaker, (*iter_order)->OrderMaker);
                    assign(pUTRtnTradeField->SessionID, (*iter_order)->SessionID);
                    assign(pUTRtnTradeField->TradeChannel, (*iter_order)->TradeChannel);

                    CUTRtnTradeFieldPtr pNewTrade{new CUTRtnTradeField{}};
                    UTCopyRtnTradeEntity(pNewTrade.get(), pUTRtnTradeField);

                    trades_.insert(pNewTrade);

                    std::cout<<"RtnTrade FIND"<<std::endl;

                    if (publisher_) publisher_->publish_package(package);
                    if (logger_) UT_LOG_INFO(logger_, "[OrderManager::RtnTrade] [OrderSysID=" << pUTRtnTradeField->OrderSysID << "]");
                    return true;
                }
                else
                {
                    std::cout<<"====================RtnTrade NOT FIND================================"<<std::endl;
                    if (logger_) UT_LOG_WARNING(logger_, "[OrderManager::RtnTrade] NO ORDER RECORD [OrderSysID=" << pUTRtnTradeField->OrderSysID << "]");
                    return false;
                }
            }
                break;
            default:
                return true;
        }
        return true;
    }

    // response create order
    bool OnRspCreateOrder(CUTRspCreateOrderField* pRspCreateOrderField)
    {
        auto& order_idx = orders_.get<Tag_OrderLocalIDIndex>();
        auto iter_order = order_idx.find(pRspCreateOrderField->OrderLocalID);
        if (iter_order != order_idx.end())
        {
            assign(pRspCreateOrderField->OrderForeID, (*iter_order)->OrderForeID);
            assign(pRspCreateOrderField->AssetType, (*iter_order)->AssetType);

            assign((*iter_order)->OrderStatus, pRspCreateOrderField->OrderStatus);
            if (strcmp(pRspCreateOrderField->OrderSysID, ""))
                assign((*iter_order)->OrderSysID, pRspCreateOrderField->OrderSysID);
            ERASE_COMPLETE_ORDER(pRspCreateOrderField);
            return true;
        }
        else
        {
            return false;
        }
    }

    // response cancel order
    bool OnRspCancelOrder(CUTRspCancelOrderField* pRspCancelOrderField)
    {
        auto& order_idx = orders_.get<Tag_OrderLocalIDIndex>();
        auto iter_order = order_idx.find(pRspCancelOrderField->OrderLocalID);
        if (iter_order != order_idx.end())
        {
            assign(pRspCancelOrderField->OrderForeID, (*iter_order)->OrderForeID);
            assign(pRspCancelOrderField->Price, (*iter_order)->Price);
            assign(pRspCancelOrderField->Volume, (*iter_order)->Volume);
            assign(pRspCancelOrderField->AssetType, (*iter_order)->AssetType);
            assign((*iter_order)->OrderStatus, pRspCancelOrderField->OrderStatus);
            ERASE_COMPLETE_ORDER(pRspCancelOrderField);
            return true;
        }
        else
        {
            return false;
        }
    }

    // return order
    bool OnRtnOrder(CUTRtnOrderField* pRtnOrderField)
    {
        auto& order_idx = orders_.get<Tag_OrderSysIDIndex>();
        auto iter_order = order_idx.find(pRtnOrderField->OrderSysID);
        if (iter_order != order_idx.end())
        {
            assign(pRtnOrderField->OrderForeID, (*iter_order)->OrderForeID);
            assign(pRtnOrderField->Price, (*iter_order)->Price);
            assign(pRtnOrderField->Volume, (*iter_order)->Volume);
            assign(pRtnOrderField->LandTime, (*iter_order)->LandTime);
            assign(pRtnOrderField->SendTime, (*iter_order)->SendTime);
            assign(pRtnOrderField->AssetType, (*iter_order)->AssetType);
            assign(pRtnOrderField->SessionID, (*iter_order)->SessionID);
            assign(pRtnOrderField->TradeChannel, (*iter_order)->TradeChannel);

            assign((*iter_order)->OrderStatus, pRtnOrderField->OrderStatus);
            assign((*iter_order)->TradeVolume, (*iter_order)->TradeVolume+(*iter_order)->TradeVolume);

            ERASE_COMPLETE_ORDER(pRtnOrderField);
            return true;
        }
        else
        {
            return false;
        }
    }

    // return trade
    bool OnRtnTrade(CUTRtnTradeField* pRtnTradeField)
    {
        auto& order_idx = orders_.get<Tag_OrderSysIDIndex>();
        auto iter_order = order_idx.find(pRtnTradeField->OrderSysID);
        if (iter_order != order_idx.end())
        {
            assign(pRtnTradeField->Price, (*iter_order)->Price);
            assign(pRtnTradeField->AssetType, (*iter_order)->AssetType);
            assign(pRtnTradeField->OrderMaker, (*iter_order)->OrderMaker);
            assign(pRtnTradeField->SessionID, (*iter_order)->SessionID);
            assign(pRtnTradeField->TradeChannel, (*iter_order)->TradeChannel);

            CUTRtnTradeFieldPtr pNewTrade{new CUTRtnTradeField{}};
            UTCopyRtnTradeEntity(pNewTrade.get(), pRtnTradeField);

            trades_.insert(pNewTrade);
            return true;
        }
        else
        {
            return false;
        }
    }

//    string retrieve_ordersysid(const string& order_local_id)
//    {
//        auto& order_idx = orders_.get<idx_ordersysid>();
//        auto iter_order = order_idx.find(order_local_id);
//        if (iter_order != order_idx.end())
//        {
//            return (*iter_order)->OrderSysID;
//        }
//        return "";
//    }

    // 收集所有报单信息
    void collect_orders(std::vector<CUTRtnOrderFieldPtr>& orders)
    {
        auto& order_idx = orders_.get<SequenceIndex>();
        for (auto iter_order=order_idx.begin(); iter_order!=order_idx.end(); ++iter_order)
        {
            orders.emplace_back(*iter_order);
        }
    }

    // 收集所有成交信息
    void collect_trades(std::vector<CUTRtnTradeFieldPtr>& trades)
    {
        auto& trade_idx = trades_.get<SequenceIndex>();
        for (auto iter_trade=trade_idx.begin(); iter_trade!=trade_idx.end(); ++iter_trade)
        {
            trades.emplace_back(*iter_trade);
        }
    }

    // 初始化缓存的报单
    void init_order(CUTRtnOrderField* pRtnOrderField)
    {
        CUTRtnOrderFieldPtr pNewOrder{new CUTRtnOrderField{}};
        UTCopyRtnOrderEntity(pNewOrder.get(), pRtnOrderField);
        orders_.insert(pNewOrder);
    }

    // 初始化缓存的成交
    void init_trade(CUTRtnTradeField* pRtnTradeField)
    {
        CUTRtnTradeFieldPtr pNewTrade{new CUTRtnTradeField{}};
        UTCopyRtnTradeEntity(pNewTrade.get(), pRtnTradeField);
        trades_.insert(pNewTrade);
    }

    // 获取有效报单
    void collect_orders(std::vector<CUTFuncRtnOrderFieldPtr>& orders)
    {
        auto& order_idx = orders_.get<SequenceIndex>();
        for (auto iter_order=order_idx.begin(); iter_order!=order_idx.end(); ++iter_order)
        {
            CUTFuncRtnOrderFieldPtr order = boost::make_shared<CUTFuncRtnOrderField>();
            UTCopyRtnOrderEntity(order.get(), (*iter_order).get());
            orders.emplace_back(order);
        }
    }
private:
    UTLogPtr logger_{nullptr};
    // publisher
    RedisPublisherPtr publisher_{nullptr};
    // order container
    Order_Container orders_;
    // trade container
    Trade_Container trades_;
};

PANDORA_NAMESPACE_END