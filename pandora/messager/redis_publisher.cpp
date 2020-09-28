/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UTType.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file redis_publisher.cpp
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190723      创建该文件
/////////////////////////////////////////////////////////////////////////

#include "redis_publisher.h"
#include "../redis/redis_api.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "quark/cxx/stg/StgPackageDesc.h"
#include "quark/cxx/stg/StgData.h"
#include "../util/json.hpp"
#include "assign.h"
#include "quark/cxx/Utils.h"


//判断字符串中是否有rtn字段

//判断字符串中是否有特定字符串




USING_PANDORA_NAMESPACE
using namespace std;


RedisPublisher::RedisPublisher(io_service_pool& pool, const std::string& srv_name) : ThreadBasePool{pool}, service_name_{srv_name}, timer_{new boost::asio::deadline_timer{get_io_service()}}
{
    get_io_service().post(std::bind(&RedisPublisher::on_timer, this, 1500));
    // on_timer(1500);
}

RedisPublisher::~RedisPublisher()
{

}

void RedisPublisher::set_server_config(const string& host, int port, const string& auth, UTLogPtr logger)
{
    logger_ = logger;
    // generate redis api
    redis_api_ = RedisApiPublishPtr{new utrade::pandora::CRedisApiPublish{logger_, pool()}};
    // redis connector
    redis_api_->RegisterRedis(host, port, auth, utrade::pandora::RM_Publish);
    // try to start receiver component
    UT_LOG_INFO(logger, "[RedisPublisher] Start Redis Host: " << host << " Port: " << port);
}

void RedisPublisher::publish_message(const string& channel_name, const string& message)
{
    std::string new_channel_name{service_name_.empty() ? channel_name : service_name_+"."+channel_name};
    boost::shared_ptr<string> channel_copy{new string{new_channel_name}};
    boost::shared_ptr<string> message_copy{new string{message}};
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_message, this, channel_copy, message_copy));
}

void RedisPublisher::handle_publish_message(boost::shared_ptr<string> channel_name, boost::shared_ptr<string> content)
{
    if (service_name_.empty())
        redis_api_->Publish(channel_name, content);
    else
        redis_api_->Publish(boost::make_shared<string>(service_name_+"."+*channel_name), content);
}

void RedisPublisher::on_timer(int millisecond)
{
    publish_buffer_content();
    // timer invoke some microseconds later
    timer_->expires_from_now(boost::posix_time::milliseconds(millisecond));
    // call the asys call function
    timer_->async_wait(boost::bind(&RedisPublisher::on_timer, this, millisecond));
}

void RedisPublisher::publish(unsigned int channel_ident, boost::shared_ptr<string> content)
{
    boost::shared_ptr<string> channel = publish_type_str(channel_ident);
    assert(channel);
    if (service_name_.empty())
        redis_api_->Publish(channel, content);
    else
        redis_api_->Publish(boost::make_shared<string>(service_name_+"."+*channel), content);
}

void RedisPublisher::publish(unsigned int channel_ident, const string& content_str)
{
    boost::shared_ptr<string> channel = publish_type_str(channel_ident);
    boost::shared_ptr<string> content{new string{content_str}};
    assert(channel);
    assert(redis_api_ && redis_api_.get());
    redis_api_->Publish(channel, content);
}

boost::shared_ptr<string> RedisPublisher::publish_type_str(unsigned int type)
{
    switch (type)
    {
        case UT_TID_RspInfo:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspInfo" : service_name_+"."+"RspInfo"}};
        case UT_TID_ReqCreateOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqCreateOrder" : service_name_+"."+"ReqCreateOrder"}};
        case UT_TID_RspCreateOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspCreateOrder" : service_name_+"."+"RspCreateOrder"}};
        case UT_TID_RtnOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnOrder" : service_name_+"."+"RtnOrder"}};
        case UT_TID_RtnTrade:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnTrade" : service_name_+"."+"RtnTrade"}};
        case UT_TID_ReqCancelOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqCancelOrder" : service_name_+"."+"ReqCancelOrder"}};
        case UT_TID_RspCancelOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspCancelOrder" : service_name_+"."+"RspCancelOrder"}};
        case UT_TID_SubPosition:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "SubPosition" : service_name_+"."+"SubPosition"}};
        case UT_TID_RtnPosition:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnPosition" : service_name_+"."+"RtnPosition"}};
        case UT_TID_ReqQryOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryOrder" : service_name_+"."+"ReqQryOrder"}};
        case UT_TID_RspQryOrder:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryOrder" : service_name_+"."+"RspQryOrder"}};
        case UT_TID_ReqQryTrade:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryTrade" : service_name_+"."+"ReqQryTrade"}};
        case UT_TID_RspQryTrade:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryTrade" : service_name_+"."+"RspQryTrade"}};
        case UT_TID_ReqQryAccount:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryAccount" : service_name_+"."+"ReqQryAccount"}};
        case UT_TID_RspQryAccount:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryAccount" : service_name_+"."+"RspQryAccount"}};
        case UT_TID_SubAccount:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "SubAccount" : service_name_+"."+"SubAccount"}};
        case UT_TID_RtnAccount:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnAccount" : service_name_+"."+"RtnAccount"}};
        case UT_TID_ReqLogin:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqLogin" : service_name_+"."+"ReqLogin"}};
        case UT_TID_RspLogin:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspLogin" : service_name_+"."+"RspLogin"}};
        case UT_TID_ReqLogout:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqLogout" : service_name_+"."+"ReqLogout"}};
        case UT_TID_RspLogout:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspLogout" : service_name_+"."+"RspLogout"}};
        case UT_TID_ReqQryPosition:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryPosition" : service_name_+"."+"ReqQryPosition"}};
        case UT_TID_RspQryPosition:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryPosition" : service_name_+"."+"RspQryPosition"}};
        case UT_TID_RtnPlatformDetail:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnPlatformDetail" : service_name_+"."+"RtnPlatformDetail"}};
        case UT_TID_RtnStrategyDetail:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnStrategyDetail" : service_name_+"."+"RtnStrategyDetail"}};
        case UT_TID_RtnDepth:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnDepth" : service_name_+"."+"RtnDepth"}};
        case UT_TID_RtnL2Depth:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnL2Depth" : service_name_+"."+"RtnL2Depth"}};
        case UT_TID_RtnL2Trade:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnL2Trade" : service_name_+"."+"RtnL2Trade"}};
        case UT_TID_RtnL2Order:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnL2Order" : service_name_+"."+"RtnL2Order"}};
        case UT_TID_RtnL2Index:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnL2Index" : service_name_+"."+"RtnL2Index"}};
        case UT_TID_RtnBarMarketData:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnBarMarketData" : service_name_+"."+"RtnBarMarketData"}};
        case UT_TID_RtnBusinessDebt:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnBusinessDebt" : service_name_+"."+"RtnBusinessDebt"}};
        case UT_TID_ReqQryAccountBusiness:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryAccountBusiness" : service_name_+"."+"ReqQryAccountBusiness"}};
        case UT_TID_RspQryAccountBusiness:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryAccountBusiness" : service_name_+"."+"RspQryAccountBusiness"}};
        case UT_TID_RtnAccountBusiness:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnAccountBusiness" : service_name_+"."+"RtnAccountBusiness"}};
        case UT_TID_ReqManualTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqManualTransact" : service_name_+"."+"ReqManualTransact"}};
        case UT_TID_ReqTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqTransact" : service_name_+"."+"ReqTransact"}};
        case UT_TID_RspTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspTransact" : service_name_+"."+"RspTransact"}};
        case UT_TID_RtnTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RtnTransact" : service_name_+"."+"RtnTransact"}};
        case UT_TID_ReqQryTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "ReqQryTransact" : service_name_+"."+"ReqQryTransact"}};
        case UT_TID_RspQryTransact:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "RspQryTransact" : service_name_+"."+"RspQryTransact"}};
        default:
            UT_LOG_ERROR_FMT(logger_, "Publish Type Error: %d ", type);
            return nullptr;
    }
}

void RedisPublisher::publish_package(PackagePtr package)
{
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_package, this, package));
}

void RedisPublisher::publish_account(boost::shared_ptr<CUTRtnAccountField> pAccount)
{
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_account, this, pAccount));
}

void RedisPublisher::publish_position(boost::shared_ptr<CUTRtnPositionField> pPosition)
{
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_position, this, pPosition));
}

void RedisPublisher::publish_business_debt(boost::shared_ptr<CUTRtnBusinessDebtField> pBusinessDebt)
{
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_business_debt, this, pBusinessDebt));
}

void RedisPublisher::publish_account_nodelay(boost::shared_ptr<CUTRtnAccountField> pAccount)
{
    get_io_service().post(std::bind(&RedisPublisher::handle_publish_account_nodelay, this, pAccount));
}

void RedisPublisher::handle_publish_package(PackagePtr package)
{
    switch(package->Tid())
    {
        case UT_TID_RspInfo:
            handle_rsp_info(package);
            break;    
        case UT_TID_ReqCreateOrder:
            handle_req_create_order(package);
            break;    
        case UT_TID_RspCreateOrder:
            handle_rsp_create_order(package);
            break;    
        case UT_TID_RtnOrder:
            handle_rtn_order(package);
            break;    
        case UT_TID_RtnTrade:
            handle_rtn_trade(package);
            break;    
        case UT_TID_ReqCancelOrder:
            handle_req_cancel_order(package);
            break;    
        case UT_TID_RspCancelOrder:
            handle_rsp_cancel_order(package);
            break;    
        case UT_TID_SubPosition:
            handle_sub_position(package);
            break;    
        case UT_TID_RtnPosition:
            handle_rtn_position(package);
            break;    
        case UT_TID_ReqQryOrder:
            handle_req_qry_order(package);
            break;    
        case UT_TID_RspQryOrder:
            handle_rsp_qry_order(package);
            break;    
        case UT_TID_ReqQryTrade:
            handle_req_qry_trade(package);
            break;    
        case UT_TID_RspQryTrade:
            handle_rsp_qry_trade(package);
            break;    
        case UT_TID_ReqQryAccount:
            handle_req_qry_account(package);
            break;    
        case UT_TID_RspQryAccount:
            handle_rsp_qry_account(package);
            break;    
        case UT_TID_SubAccount:
            handle_sub_account(package);
            break;    
        case UT_TID_RtnAccount:
            handle_rtn_account(package);
            break;    
        case UT_TID_ReqLogin:
            handle_req_login(package);
            break;    
        case UT_TID_RspLogin:
            handle_rsp_login(package);
            break;    
        case UT_TID_ReqLogout:
            handle_req_logout(package);
            break;    
        case UT_TID_RspLogout:
            handle_rsp_logout(package);
            break;    
        case UT_TID_ReqQryPosition:
            handle_req_qry_position(package);
            break;    
        case UT_TID_RspQryPosition:
            handle_rsp_qry_position(package);
            break;    
        case UT_TID_RtnPlatformDetail:
            handle_rtn_platform_detail(package);
            break;    
        case UT_TID_RtnStrategyDetail:
            handle_rtn_strategy_detail(package);
            break;    
        case UT_TID_RtnDepth:
            handle_rtn_depth(package);
            break;    
        case UT_TID_RtnL2Depth:
            handle_rtn_l2_depth(package);
            break;    
        case UT_TID_RtnL2Trade:
            handle_rtn_l2_trade(package);
            break;    
        case UT_TID_RtnL2Order:
            handle_rtn_l2_order(package);
            break;    
        case UT_TID_RtnL2Index:
            handle_rtn_l2_index(package);
            break;    
        case UT_TID_RtnBarMarketData:
            handle_rtn_bar_market_data(package);
            break;    
        case UT_TID_RtnBusinessDebt:
            handle_rtn_business_debt(package);
            break;    
        case UT_TID_ReqQryAccountBusiness:
            handle_req_qry_account_business(package);
            break;    
        case UT_TID_RspQryAccountBusiness:
            handle_rsp_qry_account_business(package);
            break;    
        case UT_TID_RtnAccountBusiness:
            handle_rtn_account_business(package);
            break;    
        case UT_TID_ReqManualTransact:
            handle_req_manual_transact(package);
            break;    
        case UT_TID_ReqTransact:
            handle_req_transact(package);
            break;    
        case UT_TID_RspTransact:
            handle_rsp_transact(package);
            break;    
        case UT_TID_RtnTransact:
            handle_rtn_transact(package);
            break;    
        case UT_TID_ReqQryTransact:
            handle_req_qry_transact(package);
            break;    
        case UT_TID_RspQryTransact:
            handle_rsp_qry_transact(package);
            break;    
        default:
            UT_LOG_ERROR_FMT(logger_, "Publish Type Error: %d ", package->Tid());
    }
}

void RedisPublisher::handle_publish_account(boost::shared_ptr<CUTRtnAccountField> pAccount)
{
    auto iter_find_key = account_buffer_.find(pAccount->CurrencyID);
    if (iter_find_key!= account_buffer_.end())
    {
        iter_find_key->second = pAccount;
    }
    else
    {
        account_buffer_.emplace(pAccount->CurrencyID, pAccount);
    }
}
    
void RedisPublisher::handle_publish_position(boost::shared_ptr<CUTRtnPositionField> pPosition)
{
    auto iter_find_key = position_buffer_.find(pPosition->PositionID);
    if (iter_find_key!= position_buffer_.end())
    {
        iter_find_key->second = pPosition;
    }
    else
    {
        position_buffer_.emplace(pPosition->PositionID, pPosition);
    }
}



void RedisPublisher::handle_publish_business_debt(boost::shared_ptr<CUTRtnBusinessDebtField> pRtnBusinessDebtField)
{
    //business_debt_buffer_.emplace_back(pBusinessDebt);
    //直接publish 不缓冲吧

    nlohmann::json RtnBusinessDebtFieldJson;
    assign(RtnBusinessDebtFieldJson["ExchangeID"], pRtnBusinessDebtField->ExchangeID);
    assign(RtnBusinessDebtFieldJson["AccountName"], pRtnBusinessDebtField->AccountName);
    assign(RtnBusinessDebtFieldJson["AccountType"], getAccountTypeString(pRtnBusinessDebtField->AccountType));
    assign(RtnBusinessDebtFieldJson["CurrencyName"], pRtnBusinessDebtField->CurrencyName);
    assign(RtnBusinessDebtFieldJson["CurrBusinessName"], pRtnBusinessDebtField->CurrBusinessName);
    assign(RtnBusinessDebtFieldJson["DebtBusinessName"], pRtnBusinessDebtField->DebtBusinessName);
    assign(RtnBusinessDebtFieldJson["DebtDirection"], getDebtDirectionString(pRtnBusinessDebtField->DebtDirection));
    assign(RtnBusinessDebtFieldJson["DebtAmount"], pRtnBusinessDebtField->DebtAmount);
    assign(RtnBusinessDebtFieldJson["CreateTime"], pRtnBusinessDebtField->CreateTime);
    boost::shared_ptr<string> content{new string{RtnBusinessDebtFieldJson.dump()}};


    publish(UT_TID_RtnBusinessDebt, content);
}

void RedisPublisher::handle_publish_account_nodelay(boost::shared_ptr<CUTRtnAccountField> pRtnAccountField)
{

    nlohmann::json RtnAccountFieldJson;
    assign(RtnAccountFieldJson["ExchangeID"], pRtnAccountField->ExchangeID);
    assign(RtnAccountFieldJson["AccountName"], pRtnAccountField->AccountName);
    assign(RtnAccountFieldJson["AccountType"], getAccountTypeString(pRtnAccountField->AccountType));
    assign(RtnAccountFieldJson["CurrencyName"], pRtnAccountField->CurrencyName);
    assign(RtnAccountFieldJson["CurrencyQuantity"], pRtnAccountField->CurrencyQuantity);
    assign(RtnAccountFieldJson["PositionMargin"], pRtnAccountField->PositionMargin);
    assign(RtnAccountFieldJson["OrderMargin"], pRtnAccountField->OrderMargin);
    assign(RtnAccountFieldJson["PositionBalance"], pRtnAccountField->PositionBalance);
    assign(RtnAccountFieldJson["TotalBalance"], pRtnAccountField->TotalBalance);
    assign(RtnAccountFieldJson["Available"], pRtnAccountField->Available);
    assign(RtnAccountFieldJson["LongAvailable"], pRtnAccountField->LongAvailable);
    assign(RtnAccountFieldJson["ShortAvailable"], pRtnAccountField->ShortAvailable);
    assign(RtnAccountFieldJson["ActualLongAvail"], pRtnAccountField->ActualLongAvail);
    assign(RtnAccountFieldJson["ActualShortAvail"], pRtnAccountField->ActualShortAvail);
    assign(RtnAccountFieldJson["Frozen"], pRtnAccountField->Frozen);
    assign(RtnAccountFieldJson["Fee"], pRtnAccountField->Fee);
    assign(RtnAccountFieldJson["FrozenBuy"], pRtnAccountField->FrozenBuy);
    assign(RtnAccountFieldJson["FrozenSell"], pRtnAccountField->FrozenSell);
    assign(RtnAccountFieldJson["UpdateTime"], pRtnAccountField->UpdateTime);
    assign(RtnAccountFieldJson["CurrencyID"], pRtnAccountField->CurrencyID);
    assign(RtnAccountFieldJson["AssetType"], getAssetTypeString(pRtnAccountField->AssetType));
    assign(RtnAccountFieldJson["TradeChannel"], pRtnAccountField->TradeChannel);
    assign(RtnAccountFieldJson["Borrow"], pRtnAccountField->Borrow);
    assign(RtnAccountFieldJson["Lend"], pRtnAccountField->Lend);
    assign(RtnAccountFieldJson["DebtOffset"], pRtnAccountField->DebtOffset);
    assign(RtnAccountFieldJson["TransferOffset"], pRtnAccountField->TransferOffset);
    boost::shared_ptr<string> content{new string{RtnAccountFieldJson.dump()}};

    publish(UT_TID_RtnAccount, content);
}

void RedisPublisher::handle_rsp_info(PackagePtr package)
{
    nlohmann::json RspInfoFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspInfo)};

    assign(RspInfoFieldJson["SequenceNo"], sequence_no);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    handleRspInfoField(RspInfoFieldJson, pRspInfoField);

    //std::cout<<RspInfoFieldJson.dump()<<std::endl;
    publish(UT_TID_RspInfo, RspInfoFieldJson.dump());
}

void RedisPublisher::handle_req_create_order(PackagePtr package)
{
    nlohmann::json ReqCreateOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqCreateOrder)};

    assign(ReqCreateOrderFieldJson["SequenceNo"], sequence_no);
    const CUTReqCreateOrderField* pReqCreateOrderField = GET_FIELD(package, CUTReqCreateOrderField);
    long requestID{package->RequestID()};
    assign(ReqCreateOrderFieldJson["RequestID"], requestID);
    handleReqCreateOrderField(ReqCreateOrderFieldJson, pReqCreateOrderField);

    //std::cout<<ReqCreateOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqCreateOrder, ReqCreateOrderFieldJson.dump());
}

void RedisPublisher::handle_rsp_create_order(PackagePtr package)
{
    nlohmann::json RspCreateOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspCreateOrder)};

    assign(RspCreateOrderFieldJson["SequenceNo"], sequence_no);
    const CUTRspCreateOrderField* pRspCreateOrderField = GET_FIELD(package, CUTRspCreateOrderField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspCreateOrderFieldJson["RequestID"], requestID);
    handleRspCreateOrderField(RspCreateOrderFieldJson, pRspCreateOrderField, pRspInfoField);

    //std::cout<<RspCreateOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_RspCreateOrder, RspCreateOrderFieldJson.dump());
}

void RedisPublisher::handle_rtn_order(PackagePtr package)
{
    nlohmann::json RtnOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnOrder)};

    assign(RtnOrderFieldJson["SequenceNo"], sequence_no);
    const CUTRtnOrderField* pRtnOrderField = GET_FIELD(package, CUTRtnOrderField);
    handleRtnOrderField(RtnOrderFieldJson, pRtnOrderField);

    //std::cout<<RtnOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnOrder, RtnOrderFieldJson.dump());
}

void RedisPublisher::handle_rtn_trade(PackagePtr package)
{
    nlohmann::json RtnTradeFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnTrade)};

    assign(RtnTradeFieldJson["SequenceNo"], sequence_no);
    const CUTRtnTradeField* pRtnTradeField = GET_FIELD(package, CUTRtnTradeField);
    handleRtnTradeField(RtnTradeFieldJson, pRtnTradeField);

    //std::cout<<RtnTradeFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnTrade, RtnTradeFieldJson.dump());
}

void RedisPublisher::handle_req_cancel_order(PackagePtr package)
{
    nlohmann::json ReqCancelOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqCancelOrder)};

    assign(ReqCancelOrderFieldJson["SequenceNo"], sequence_no);
    const CUTReqCancelOrderField* pReqCancelOrderField = GET_FIELD(package, CUTReqCancelOrderField);
    long requestID{package->RequestID()};
    assign(ReqCancelOrderFieldJson["RequestID"], requestID);
    handleReqCancelOrderField(ReqCancelOrderFieldJson, pReqCancelOrderField);

    //std::cout<<ReqCancelOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqCancelOrder, ReqCancelOrderFieldJson.dump());
}

void RedisPublisher::handle_rsp_cancel_order(PackagePtr package)
{
    nlohmann::json RspCancelOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspCancelOrder)};

    assign(RspCancelOrderFieldJson["SequenceNo"], sequence_no);
    const CUTRspCancelOrderField* pRspCancelOrderField = GET_FIELD(package, CUTRspCancelOrderField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspCancelOrderFieldJson["RequestID"], requestID);
    handleRspCancelOrderField(RspCancelOrderFieldJson, pRspCancelOrderField, pRspInfoField);

    //std::cout<<RspCancelOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_RspCancelOrder, RspCancelOrderFieldJson.dump());
}

void RedisPublisher::handle_sub_position(PackagePtr package)
{
    nlohmann::json SubPositionFieldJson;
    long long sequence_no{get_publish_id(UT_TID_SubPosition)};

    assign(SubPositionFieldJson["SequenceNo"], sequence_no);
    const CUTSubPositionField* pSubPositionField = GET_FIELD(package, CUTSubPositionField);
    handleSubPositionField(SubPositionFieldJson, pSubPositionField);

    //std::cout<<SubPositionFieldJson.dump()<<std::endl;
    publish(UT_TID_SubPosition, SubPositionFieldJson.dump());
}

void RedisPublisher::handle_rtn_position(PackagePtr package)
{
    nlohmann::json RtnPositionFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnPosition)};

    assign(RtnPositionFieldJson["SequenceNo"], sequence_no);
    const CUTRtnPositionField* pRtnPositionField = GET_FIELD(package, CUTRtnPositionField);
    handleRtnPositionField(RtnPositionFieldJson, pRtnPositionField);

    //std::cout<<RtnPositionFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnPosition, RtnPositionFieldJson.dump());
}

void RedisPublisher::handle_req_qry_order(PackagePtr package)
{
    nlohmann::json ReqQryOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryOrder)};

    assign(ReqQryOrderFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryOrderField* pReqQryOrderField = GET_FIELD(package, CUTReqQryOrderField);
    long requestID{package->RequestID()};
    assign(ReqQryOrderFieldJson["RequestID"], requestID);
    handleReqQryOrderField(ReqQryOrderFieldJson, pReqQryOrderField);

    //std::cout<<ReqQryOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryOrder, ReqQryOrderFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_order(PackagePtr package)
{
    nlohmann::json RspQryOrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryOrder)};

    assign(RspQryOrderFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryOrderField* pRspQryOrderField = GET_FIELD(package, CUTRspQryOrderField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryOrderFieldJson["RequestID"], requestID);
    handleRspQryOrderField(RspQryOrderFieldJson, pRspQryOrderField, pRspInfoField);

    //std::cout<<RspQryOrderFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryOrder, RspQryOrderFieldJson.dump());
}

void RedisPublisher::handle_req_qry_trade(PackagePtr package)
{
    nlohmann::json ReqQryTradeFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryTrade)};

    assign(ReqQryTradeFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryTradeField* pReqQryTradeField = GET_FIELD(package, CUTReqQryTradeField);
    long requestID{package->RequestID()};
    assign(ReqQryTradeFieldJson["RequestID"], requestID);
    handleReqQryTradeField(ReqQryTradeFieldJson, pReqQryTradeField);

    //std::cout<<ReqQryTradeFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryTrade, ReqQryTradeFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_trade(PackagePtr package)
{
    nlohmann::json RspQryTradeFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryTrade)};

    assign(RspQryTradeFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryTradeField* pRspQryTradeField = GET_FIELD(package, CUTRspQryTradeField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryTradeFieldJson["RequestID"], requestID);
    handleRspQryTradeField(RspQryTradeFieldJson, pRspQryTradeField, pRspInfoField);

    //std::cout<<RspQryTradeFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryTrade, RspQryTradeFieldJson.dump());
}

void RedisPublisher::handle_req_qry_account(PackagePtr package)
{
    nlohmann::json ReqQryAccountFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryAccount)};

    assign(ReqQryAccountFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryAccountField* pReqQryAccountField = GET_FIELD(package, CUTReqQryAccountField);
    long requestID{package->RequestID()};
    assign(ReqQryAccountFieldJson["RequestID"], requestID);
    handleReqQryAccountField(ReqQryAccountFieldJson, pReqQryAccountField);

    //std::cout<<ReqQryAccountFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryAccount, ReqQryAccountFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_account(PackagePtr package)
{
    nlohmann::json RspQryAccountFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryAccount)};

    assign(RspQryAccountFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryAccountField* pRspQryAccountField = GET_FIELD(package, CUTRspQryAccountField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryAccountFieldJson["RequestID"], requestID);
    handleRspQryAccountField(RspQryAccountFieldJson, pRspQryAccountField, pRspInfoField);

    //std::cout<<RspQryAccountFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryAccount, RspQryAccountFieldJson.dump());
}

void RedisPublisher::handle_sub_account(PackagePtr package)
{
    nlohmann::json SubAccountFieldJson;
    long long sequence_no{get_publish_id(UT_TID_SubAccount)};

    assign(SubAccountFieldJson["SequenceNo"], sequence_no);
    const CUTSubAccountField* pSubAccountField = GET_FIELD(package, CUTSubAccountField);
    handleSubAccountField(SubAccountFieldJson, pSubAccountField);

    //std::cout<<SubAccountFieldJson.dump()<<std::endl;
    publish(UT_TID_SubAccount, SubAccountFieldJson.dump());
}

void RedisPublisher::handle_rtn_account(PackagePtr package)
{
    nlohmann::json RtnAccountFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnAccount)};

    assign(RtnAccountFieldJson["SequenceNo"], sequence_no);
    const CUTRtnAccountField* pRtnAccountField = GET_FIELD(package, CUTRtnAccountField);
    handleRtnAccountField(RtnAccountFieldJson, pRtnAccountField);

    //std::cout<<RtnAccountFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnAccount, RtnAccountFieldJson.dump());
}

void RedisPublisher::handle_req_login(PackagePtr package)
{
    nlohmann::json ReqLoginFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqLogin)};

    assign(ReqLoginFieldJson["SequenceNo"], sequence_no);
    const CUTReqLoginField* pReqLoginField = GET_FIELD(package, CUTReqLoginField);
    long requestID{package->RequestID()};
    assign(ReqLoginFieldJson["RequestID"], requestID);
    handleReqLoginField(ReqLoginFieldJson, pReqLoginField);

    //std::cout<<ReqLoginFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqLogin, ReqLoginFieldJson.dump());
}

void RedisPublisher::handle_rsp_login(PackagePtr package)
{
    nlohmann::json RspLoginFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspLogin)};

    assign(RspLoginFieldJson["SequenceNo"], sequence_no);
    const CUTRspLoginField* pRspLoginField = GET_FIELD(package, CUTRspLoginField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspLoginFieldJson["RequestID"], requestID);
    handleRspLoginField(RspLoginFieldJson, pRspLoginField, pRspInfoField);

    //std::cout<<RspLoginFieldJson.dump()<<std::endl;
    publish(UT_TID_RspLogin, RspLoginFieldJson.dump());
}

void RedisPublisher::handle_req_logout(PackagePtr package)
{
    nlohmann::json ReqLogoutFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqLogout)};

    assign(ReqLogoutFieldJson["SequenceNo"], sequence_no);
    const CUTReqLogoutField* pReqLogoutField = GET_FIELD(package, CUTReqLogoutField);
    long requestID{package->RequestID()};
    assign(ReqLogoutFieldJson["RequestID"], requestID);
    handleReqLogoutField(ReqLogoutFieldJson, pReqLogoutField);

    //std::cout<<ReqLogoutFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqLogout, ReqLogoutFieldJson.dump());
}

void RedisPublisher::handle_rsp_logout(PackagePtr package)
{
    nlohmann::json RspLogoutFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspLogout)};

    assign(RspLogoutFieldJson["SequenceNo"], sequence_no);
    const CUTRspLogoutField* pRspLogoutField = GET_FIELD(package, CUTRspLogoutField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspLogoutFieldJson["RequestID"], requestID);
    handleRspLogoutField(RspLogoutFieldJson, pRspLogoutField, pRspInfoField);

    //std::cout<<RspLogoutFieldJson.dump()<<std::endl;
    publish(UT_TID_RspLogout, RspLogoutFieldJson.dump());
}

void RedisPublisher::handle_req_qry_position(PackagePtr package)
{
    nlohmann::json ReqQryPositionFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryPosition)};

    assign(ReqQryPositionFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryPositionField* pReqQryPositionField = GET_FIELD(package, CUTReqQryPositionField);
    long requestID{package->RequestID()};
    assign(ReqQryPositionFieldJson["RequestID"], requestID);
    handleReqQryPositionField(ReqQryPositionFieldJson, pReqQryPositionField);

    //std::cout<<ReqQryPositionFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryPosition, ReqQryPositionFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_position(PackagePtr package)
{
    nlohmann::json RspQryPositionFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryPosition)};

    assign(RspQryPositionFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryPositionField* pRspQryPositionField = GET_FIELD(package, CUTRspQryPositionField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryPositionFieldJson["RequestID"], requestID);
    handleRspQryPositionField(RspQryPositionFieldJson, pRspQryPositionField, pRspInfoField);

    //std::cout<<RspQryPositionFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryPosition, RspQryPositionFieldJson.dump());
}

void RedisPublisher::handle_rtn_platform_detail(PackagePtr package)
{
    nlohmann::json RtnPlatformDetailFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnPlatformDetail)};

    assign(RtnPlatformDetailFieldJson["SequenceNo"], sequence_no);
    const CUTRtnPlatformDetailField* pRtnPlatformDetailField = GET_FIELD(package, CUTRtnPlatformDetailField);
    handleRtnPlatformDetailField(RtnPlatformDetailFieldJson, pRtnPlatformDetailField);

    //std::cout<<RtnPlatformDetailFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnPlatformDetail, RtnPlatformDetailFieldJson.dump());
}

void RedisPublisher::handle_rtn_strategy_detail(PackagePtr package)
{
    nlohmann::json RtnStrategyDetailFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnStrategyDetail)};

    assign(RtnStrategyDetailFieldJson["SequenceNo"], sequence_no);
    const CUTRtnStrategyDetailField* pRtnStrategyDetailField = GET_FIELD(package, CUTRtnStrategyDetailField);
    handleRtnStrategyDetailField(RtnStrategyDetailFieldJson, pRtnStrategyDetailField);

    //std::cout<<RtnStrategyDetailFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnStrategyDetail, RtnStrategyDetailFieldJson.dump());
}

void RedisPublisher::handle_rtn_depth(PackagePtr package)
{
    nlohmann::json RtnDepthFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnDepth)};

    assign(RtnDepthFieldJson["SequenceNo"], sequence_no);
    const CUTRtnDepthField* pRtnDepthField = GET_FIELD(package, CUTRtnDepthField);
    handleRtnDepthField(RtnDepthFieldJson, pRtnDepthField);

    //std::cout<<RtnDepthFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnDepth, RtnDepthFieldJson.dump());
}

void RedisPublisher::handle_rtn_l2_depth(PackagePtr package)
{
    nlohmann::json RtnL2DepthFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnL2Depth)};

    assign(RtnL2DepthFieldJson["SequenceNo"], sequence_no);
    const CUTRtnL2DepthField* pRtnL2DepthField = GET_FIELD(package, CUTRtnL2DepthField);
    handleRtnL2DepthField(RtnL2DepthFieldJson, pRtnL2DepthField);

    //std::cout<<RtnL2DepthFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnL2Depth, RtnL2DepthFieldJson.dump());
}

void RedisPublisher::handle_rtn_l2_trade(PackagePtr package)
{
    nlohmann::json RtnL2TradeFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnL2Trade)};

    assign(RtnL2TradeFieldJson["SequenceNo"], sequence_no);
    const CUTRtnL2TradeField* pRtnL2TradeField = GET_FIELD(package, CUTRtnL2TradeField);
    handleRtnL2TradeField(RtnL2TradeFieldJson, pRtnL2TradeField);

    //std::cout<<RtnL2TradeFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnL2Trade, RtnL2TradeFieldJson.dump());
}

void RedisPublisher::handle_rtn_l2_order(PackagePtr package)
{
    nlohmann::json RtnL2OrderFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnL2Order)};

    assign(RtnL2OrderFieldJson["SequenceNo"], sequence_no);
    const CUTRtnL2OrderField* pRtnL2OrderField = GET_FIELD(package, CUTRtnL2OrderField);
    handleRtnL2OrderField(RtnL2OrderFieldJson, pRtnL2OrderField);

    //std::cout<<RtnL2OrderFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnL2Order, RtnL2OrderFieldJson.dump());
}

void RedisPublisher::handle_rtn_l2_index(PackagePtr package)
{
    nlohmann::json RtnL2IndexFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnL2Index)};

    assign(RtnL2IndexFieldJson["SequenceNo"], sequence_no);
    const CUTRtnL2IndexField* pRtnL2IndexField = GET_FIELD(package, CUTRtnL2IndexField);
    handleRtnL2IndexField(RtnL2IndexFieldJson, pRtnL2IndexField);

    //std::cout<<RtnL2IndexFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnL2Index, RtnL2IndexFieldJson.dump());
}

void RedisPublisher::handle_rtn_bar_market_data(PackagePtr package)
{
    nlohmann::json RtnBarMarketDataFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnBarMarketData)};

    assign(RtnBarMarketDataFieldJson["SequenceNo"], sequence_no);
    const CUTRtnBarMarketDataField* pRtnBarMarketDataField = GET_FIELD(package, CUTRtnBarMarketDataField);
    handleRtnBarMarketDataField(RtnBarMarketDataFieldJson, pRtnBarMarketDataField);

    //std::cout<<RtnBarMarketDataFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnBarMarketData, RtnBarMarketDataFieldJson.dump());
}

void RedisPublisher::handle_rtn_business_debt(PackagePtr package)
{
    nlohmann::json RtnBusinessDebtFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnBusinessDebt)};

    assign(RtnBusinessDebtFieldJson["SequenceNo"], sequence_no);
    const CUTRtnBusinessDebtField* pRtnBusinessDebtField = GET_FIELD(package, CUTRtnBusinessDebtField);
    handleRtnBusinessDebtField(RtnBusinessDebtFieldJson, pRtnBusinessDebtField);

    //std::cout<<RtnBusinessDebtFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnBusinessDebt, RtnBusinessDebtFieldJson.dump());
}

void RedisPublisher::handle_req_qry_account_business(PackagePtr package)
{
    nlohmann::json ReqQryAccountBusinessFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryAccountBusiness)};

    assign(ReqQryAccountBusinessFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryAccountBusinessField* pReqQryAccountBusinessField = GET_FIELD(package, CUTReqQryAccountBusinessField);
    long requestID{package->RequestID()};
    assign(ReqQryAccountBusinessFieldJson["RequestID"], requestID);
    handleReqQryAccountBusinessField(ReqQryAccountBusinessFieldJson, pReqQryAccountBusinessField);

    //std::cout<<ReqQryAccountBusinessFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryAccountBusiness, ReqQryAccountBusinessFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_account_business(PackagePtr package)
{
    nlohmann::json RspQryAccountBusinessFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryAccountBusiness)};

    assign(RspQryAccountBusinessFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryAccountBusinessField* pRspQryAccountBusinessField = GET_FIELD(package, CUTRspQryAccountBusinessField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryAccountBusinessFieldJson["RequestID"], requestID);
    handleRspQryAccountBusinessField(RspQryAccountBusinessFieldJson, pRspQryAccountBusinessField, pRspInfoField);

    //std::cout<<RspQryAccountBusinessFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryAccountBusiness, RspQryAccountBusinessFieldJson.dump());
}

void RedisPublisher::handle_rtn_account_business(PackagePtr package)
{
    nlohmann::json RtnAccountBusinessFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnAccountBusiness)};

    assign(RtnAccountBusinessFieldJson["SequenceNo"], sequence_no);
    const CUTRtnAccountBusinessField* pRtnAccountBusinessField = GET_FIELD(package, CUTRtnAccountBusinessField);
    handleRtnAccountBusinessField(RtnAccountBusinessFieldJson, pRtnAccountBusinessField);

    //std::cout<<RtnAccountBusinessFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnAccountBusiness, RtnAccountBusinessFieldJson.dump());
}

void RedisPublisher::handle_req_manual_transact(PackagePtr package)
{
    nlohmann::json ReqManualTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqManualTransact)};

    assign(ReqManualTransactFieldJson["SequenceNo"], sequence_no);
    const CUTReqManualTransactField* pReqManualTransactField = GET_FIELD(package, CUTReqManualTransactField);
    long requestID{package->RequestID()};
    assign(ReqManualTransactFieldJson["RequestID"], requestID);
    handleReqManualTransactField(ReqManualTransactFieldJson, pReqManualTransactField);

    //std::cout<<ReqManualTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqManualTransact, ReqManualTransactFieldJson.dump());
}

void RedisPublisher::handle_req_transact(PackagePtr package)
{
    nlohmann::json ReqTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqTransact)};

    assign(ReqTransactFieldJson["SequenceNo"], sequence_no);
    const CUTReqTransactField* pReqTransactField = GET_FIELD(package, CUTReqTransactField);
    long requestID{package->RequestID()};
    assign(ReqTransactFieldJson["RequestID"], requestID);
    handleReqTransactField(ReqTransactFieldJson, pReqTransactField);

    //std::cout<<ReqTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqTransact, ReqTransactFieldJson.dump());
}

void RedisPublisher::handle_rsp_transact(PackagePtr package)
{
    nlohmann::json RspTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspTransact)};

    assign(RspTransactFieldJson["SequenceNo"], sequence_no);
    const CUTRspTransactField* pRspTransactField = GET_FIELD(package, CUTRspTransactField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspTransactFieldJson["RequestID"], requestID);
    handleRspTransactField(RspTransactFieldJson, pRspTransactField, pRspInfoField);

    //std::cout<<RspTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_RspTransact, RspTransactFieldJson.dump());
}

void RedisPublisher::handle_rtn_transact(PackagePtr package)
{
    nlohmann::json RtnTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RtnTransact)};

    assign(RtnTransactFieldJson["SequenceNo"], sequence_no);
    const CUTRtnTransactField* pRtnTransactField = GET_FIELD(package, CUTRtnTransactField);
    handleRtnTransactField(RtnTransactFieldJson, pRtnTransactField);

    //std::cout<<RtnTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_RtnTransact, RtnTransactFieldJson.dump());
}

void RedisPublisher::handle_req_qry_transact(PackagePtr package)
{
    nlohmann::json ReqQryTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_ReqQryTransact)};

    assign(ReqQryTransactFieldJson["SequenceNo"], sequence_no);
    const CUTReqQryTransactField* pReqQryTransactField = GET_FIELD(package, CUTReqQryTransactField);
    long requestID{package->RequestID()};
    assign(ReqQryTransactFieldJson["RequestID"], requestID);
    handleReqQryTransactField(ReqQryTransactFieldJson, pReqQryTransactField);

    //std::cout<<ReqQryTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_ReqQryTransact, ReqQryTransactFieldJson.dump());
}

void RedisPublisher::handle_rsp_qry_transact(PackagePtr package)
{
    nlohmann::json RspQryTransactFieldJson;
    long long sequence_no{get_publish_id(UT_TID_RspQryTransact)};

    assign(RspQryTransactFieldJson["SequenceNo"], sequence_no);
    const CUTRspQryTransactField* pRspQryTransactField = GET_FIELD(package, CUTRspQryTransactField);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(RspQryTransactFieldJson["RequestID"], requestID);
    handleRspQryTransactField(RspQryTransactFieldJson, pRspQryTransactField, pRspInfoField);

    //std::cout<<RspQryTransactFieldJson.dump()<<std::endl;
    publish(UT_TID_RspQryTransact, RspQryTransactFieldJson.dump());
}


void RedisPublisher::handleRspInfoField(nlohmann::json& RspInfoFieldJson, const CUTRspInfoField* pRspInfoField)
{
    assign(RspInfoFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspInfoFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleReqCreateOrderField(nlohmann::json& ReqCreateOrderFieldJson, const CUTReqCreateOrderField* pReqCreateOrderField)
{
    assign(ReqCreateOrderFieldJson["ExchangeID"], pReqCreateOrderField->ExchangeID);
    assign(ReqCreateOrderFieldJson["InstrumentID"], pReqCreateOrderField->InstrumentID);
    assign(ReqCreateOrderFieldJson["Price"], pReqCreateOrderField->Price);
    assign(ReqCreateOrderFieldJson["Volume"], pReqCreateOrderField->Volume);
    assign(ReqCreateOrderFieldJson["Direction"], getDirectionString(pReqCreateOrderField->Direction));
    assign(ReqCreateOrderFieldJson["OffsetFlag"], getOffsetFlagString(pReqCreateOrderField->OffsetFlag));
    assign(ReqCreateOrderFieldJson["OrderLocalID"], pReqCreateOrderField->OrderLocalID);
    assign(ReqCreateOrderFieldJson["OrderMaker"], getOrderMakerString(pReqCreateOrderField->OrderMaker));
    assign(ReqCreateOrderFieldJson["OrderType"], getOrderTypeString(pReqCreateOrderField->OrderType));
    assign(ReqCreateOrderFieldJson["LandTime"], pReqCreateOrderField->LandTime);
    assign(ReqCreateOrderFieldJson["SendTime"], pReqCreateOrderField->SendTime);
    assign(ReqCreateOrderFieldJson["StrategyOrderID"], pReqCreateOrderField->StrategyOrderID);
    assign(ReqCreateOrderFieldJson["OrderMode"], getOrderModeString(pReqCreateOrderField->OrderMode));
    assign(ReqCreateOrderFieldJson["AssetType"], getAssetTypeString(pReqCreateOrderField->AssetType));
    assign(ReqCreateOrderFieldJson["TradeChannel"], pReqCreateOrderField->TradeChannel);
    assign(ReqCreateOrderFieldJson["OrderXO"], getOrderXOString(pReqCreateOrderField->OrderXO));
    assign(ReqCreateOrderFieldJson["PlatformTime"], pReqCreateOrderField->PlatformTime);
    assign(ReqCreateOrderFieldJson["OrderSysID"], pReqCreateOrderField->OrderSysID);
    assign(ReqCreateOrderFieldJson["OrderForeID"], pReqCreateOrderField->OrderForeID);
    assign(ReqCreateOrderFieldJson["CreateTime"], pReqCreateOrderField->CreateTime);
    assign(ReqCreateOrderFieldJson["ModifyTime"], pReqCreateOrderField->ModifyTime);
    assign(ReqCreateOrderFieldJson["RspLocalTime"], pReqCreateOrderField->RspLocalTime);
    assign(ReqCreateOrderFieldJson["Cost"], pReqCreateOrderField->Cost);
    assign(ReqCreateOrderFieldJson["TradePrice"], pReqCreateOrderField->TradePrice);
    assign(ReqCreateOrderFieldJson["TradeVolume"], pReqCreateOrderField->TradeVolume);
    assign(ReqCreateOrderFieldJson["TradeValue"], pReqCreateOrderField->TradeValue);
    assign(ReqCreateOrderFieldJson["OrderStatus"], getOrderStatusString(pReqCreateOrderField->OrderStatus));
    assign(ReqCreateOrderFieldJson["SessionID"], pReqCreateOrderField->SessionID);
    assign(ReqCreateOrderFieldJson["RequestID"], pReqCreateOrderField->RequestID);
    assign(ReqCreateOrderFieldJson["RequestForeID"], pReqCreateOrderField->RequestForeID);
    assign(ReqCreateOrderFieldJson["Fee"], pReqCreateOrderField->Fee);
    assign(ReqCreateOrderFieldJson["FeeCurrency"], pReqCreateOrderField->FeeCurrency);
}
void RedisPublisher::handleRspCreateOrderField(nlohmann::json& RspCreateOrderFieldJson, const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspCreateOrderFieldJson["ExchangeID"], pRspCreateOrderField->ExchangeID);
    assign(RspCreateOrderFieldJson["InstrumentID"], pRspCreateOrderField->InstrumentID);
    assign(RspCreateOrderFieldJson["Price"], pRspCreateOrderField->Price);
    assign(RspCreateOrderFieldJson["Volume"], pRspCreateOrderField->Volume);
    assign(RspCreateOrderFieldJson["Direction"], getDirectionString(pRspCreateOrderField->Direction));
    assign(RspCreateOrderFieldJson["OffsetFlag"], getOffsetFlagString(pRspCreateOrderField->OffsetFlag));
    assign(RspCreateOrderFieldJson["OrderLocalID"], pRspCreateOrderField->OrderLocalID);
    assign(RspCreateOrderFieldJson["OrderMaker"], getOrderMakerString(pRspCreateOrderField->OrderMaker));
    assign(RspCreateOrderFieldJson["OrderType"], getOrderTypeString(pRspCreateOrderField->OrderType));
    assign(RspCreateOrderFieldJson["LandTime"], pRspCreateOrderField->LandTime);
    assign(RspCreateOrderFieldJson["SendTime"], pRspCreateOrderField->SendTime);
    assign(RspCreateOrderFieldJson["StrategyOrderID"], pRspCreateOrderField->StrategyOrderID);
    assign(RspCreateOrderFieldJson["OrderMode"], getOrderModeString(pRspCreateOrderField->OrderMode));
    assign(RspCreateOrderFieldJson["AssetType"], getAssetTypeString(pRspCreateOrderField->AssetType));
    assign(RspCreateOrderFieldJson["TradeChannel"], pRspCreateOrderField->TradeChannel);
    assign(RspCreateOrderFieldJson["OrderXO"], getOrderXOString(pRspCreateOrderField->OrderXO));
    assign(RspCreateOrderFieldJson["PlatformTime"], pRspCreateOrderField->PlatformTime);
    assign(RspCreateOrderFieldJson["OrderSysID"], pRspCreateOrderField->OrderSysID);
    assign(RspCreateOrderFieldJson["OrderForeID"], pRspCreateOrderField->OrderForeID);
    assign(RspCreateOrderFieldJson["CreateTime"], pRspCreateOrderField->CreateTime);
    assign(RspCreateOrderFieldJson["ModifyTime"], pRspCreateOrderField->ModifyTime);
    assign(RspCreateOrderFieldJson["RspLocalTime"], pRspCreateOrderField->RspLocalTime);
    assign(RspCreateOrderFieldJson["Cost"], pRspCreateOrderField->Cost);
    assign(RspCreateOrderFieldJson["TradePrice"], pRspCreateOrderField->TradePrice);
    assign(RspCreateOrderFieldJson["TradeVolume"], pRspCreateOrderField->TradeVolume);
    assign(RspCreateOrderFieldJson["TradeValue"], pRspCreateOrderField->TradeValue);
    assign(RspCreateOrderFieldJson["OrderStatus"], getOrderStatusString(pRspCreateOrderField->OrderStatus));
    assign(RspCreateOrderFieldJson["SessionID"], pRspCreateOrderField->SessionID);
    assign(RspCreateOrderFieldJson["RequestID"], pRspCreateOrderField->RequestID);
    assign(RspCreateOrderFieldJson["RequestForeID"], pRspCreateOrderField->RequestForeID);
    assign(RspCreateOrderFieldJson["Fee"], pRspCreateOrderField->Fee);
    assign(RspCreateOrderFieldJson["FeeCurrency"], pRspCreateOrderField->FeeCurrency);
    assign(RspCreateOrderFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspCreateOrderFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleRtnOrderField(nlohmann::json& RtnOrderFieldJson, const CUTRtnOrderField* pRtnOrderField)
{
    assign(RtnOrderFieldJson["ExchangeID"], pRtnOrderField->ExchangeID);
    assign(RtnOrderFieldJson["InstrumentID"], pRtnOrderField->InstrumentID);
    assign(RtnOrderFieldJson["Price"], pRtnOrderField->Price);
    assign(RtnOrderFieldJson["Volume"], pRtnOrderField->Volume);
    assign(RtnOrderFieldJson["Direction"], getDirectionString(pRtnOrderField->Direction));
    assign(RtnOrderFieldJson["OffsetFlag"], getOffsetFlagString(pRtnOrderField->OffsetFlag));
    assign(RtnOrderFieldJson["OrderLocalID"], pRtnOrderField->OrderLocalID);
    assign(RtnOrderFieldJson["OrderMaker"], getOrderMakerString(pRtnOrderField->OrderMaker));
    assign(RtnOrderFieldJson["OrderType"], getOrderTypeString(pRtnOrderField->OrderType));
    assign(RtnOrderFieldJson["LandTime"], pRtnOrderField->LandTime);
    assign(RtnOrderFieldJson["SendTime"], pRtnOrderField->SendTime);
    assign(RtnOrderFieldJson["StrategyOrderID"], pRtnOrderField->StrategyOrderID);
    assign(RtnOrderFieldJson["OrderMode"], getOrderModeString(pRtnOrderField->OrderMode));
    assign(RtnOrderFieldJson["AssetType"], getAssetTypeString(pRtnOrderField->AssetType));
    assign(RtnOrderFieldJson["TradeChannel"], pRtnOrderField->TradeChannel);
    assign(RtnOrderFieldJson["OrderXO"], getOrderXOString(pRtnOrderField->OrderXO));
    assign(RtnOrderFieldJson["PlatformTime"], pRtnOrderField->PlatformTime);
    assign(RtnOrderFieldJson["OrderSysID"], pRtnOrderField->OrderSysID);
    assign(RtnOrderFieldJson["OrderForeID"], pRtnOrderField->OrderForeID);
    assign(RtnOrderFieldJson["CreateTime"], pRtnOrderField->CreateTime);
    assign(RtnOrderFieldJson["ModifyTime"], pRtnOrderField->ModifyTime);
    assign(RtnOrderFieldJson["RspLocalTime"], pRtnOrderField->RspLocalTime);
    assign(RtnOrderFieldJson["Cost"], pRtnOrderField->Cost);
    assign(RtnOrderFieldJson["TradePrice"], pRtnOrderField->TradePrice);
    assign(RtnOrderFieldJson["TradeVolume"], pRtnOrderField->TradeVolume);
    assign(RtnOrderFieldJson["TradeValue"], pRtnOrderField->TradeValue);
    assign(RtnOrderFieldJson["OrderStatus"], getOrderStatusString(pRtnOrderField->OrderStatus));
    assign(RtnOrderFieldJson["SessionID"], pRtnOrderField->SessionID);
    assign(RtnOrderFieldJson["RequestID"], pRtnOrderField->RequestID);
    assign(RtnOrderFieldJson["RequestForeID"], pRtnOrderField->RequestForeID);
    assign(RtnOrderFieldJson["Fee"], pRtnOrderField->Fee);
    assign(RtnOrderFieldJson["FeeCurrency"], pRtnOrderField->FeeCurrency);
}
void RedisPublisher::handleRtnTradeField(nlohmann::json& RtnTradeFieldJson, const CUTRtnTradeField* pRtnTradeField)
{
    assign(RtnTradeFieldJson["TradeID"], pRtnTradeField->TradeID);
    assign(RtnTradeFieldJson["OrderSysID"], pRtnTradeField->OrderSysID);
    assign(RtnTradeFieldJson["ExchangeID"], pRtnTradeField->ExchangeID);
    assign(RtnTradeFieldJson["InstrumentID"], pRtnTradeField->InstrumentID);
    assign(RtnTradeFieldJson["MatchPrice"], pRtnTradeField->MatchPrice);
    assign(RtnTradeFieldJson["MatchVolume"], pRtnTradeField->MatchVolume);
    assign(RtnTradeFieldJson["MatchValue"], pRtnTradeField->MatchValue);
    assign(RtnTradeFieldJson["Direction"], getDirectionString(pRtnTradeField->Direction));
    assign(RtnTradeFieldJson["OrderLocalID"], pRtnTradeField->OrderLocalID);
    assign(RtnTradeFieldJson["Fee"], pRtnTradeField->Fee);
    assign(RtnTradeFieldJson["FeeCurrency"], pRtnTradeField->FeeCurrency);
    assign(RtnTradeFieldJson["PlatformTime"], pRtnTradeField->PlatformTime);
    assign(RtnTradeFieldJson["TradeTime"], pRtnTradeField->TradeTime);
    assign(RtnTradeFieldJson["RspLocalTime"], pRtnTradeField->RspLocalTime);
    assign(RtnTradeFieldJson["Price"], pRtnTradeField->Price);
    assign(RtnTradeFieldJson["StrategyOrderID"], pRtnTradeField->StrategyOrderID);
    assign(RtnTradeFieldJson["OrderMaker"], getOrderMakerString(pRtnTradeField->OrderMaker));
    assign(RtnTradeFieldJson["AssetType"], getAssetTypeString(pRtnTradeField->AssetType));
    assign(RtnTradeFieldJson["TradeChannel"], pRtnTradeField->TradeChannel);
    assign(RtnTradeFieldJson["SessionID"], pRtnTradeField->SessionID);
}
void RedisPublisher::handleReqCancelOrderField(nlohmann::json& ReqCancelOrderFieldJson, const CUTReqCancelOrderField* pReqCancelOrderField)
{
    assign(ReqCancelOrderFieldJson["OrderLocalID"], pReqCancelOrderField->OrderLocalID);
    assign(ReqCancelOrderFieldJson["OrderForeID"], pReqCancelOrderField->OrderForeID);
    assign(ReqCancelOrderFieldJson["OrderSysID"], pReqCancelOrderField->OrderSysID);
    assign(ReqCancelOrderFieldJson["ExchangeID"], pReqCancelOrderField->ExchangeID);
    assign(ReqCancelOrderFieldJson["StrategyOrderID"], pReqCancelOrderField->StrategyOrderID);
    assign(ReqCancelOrderFieldJson["TradeChannel"], pReqCancelOrderField->TradeChannel);
}
void RedisPublisher::handleRspCancelOrderField(nlohmann::json& RspCancelOrderFieldJson, const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspCancelOrderFieldJson["ExchangeID"], pRspCancelOrderField->ExchangeID);
    assign(RspCancelOrderFieldJson["InstrumentID"], pRspCancelOrderField->InstrumentID);
    assign(RspCancelOrderFieldJson["Price"], pRspCancelOrderField->Price);
    assign(RspCancelOrderFieldJson["Volume"], pRspCancelOrderField->Volume);
    assign(RspCancelOrderFieldJson["Direction"], getDirectionString(pRspCancelOrderField->Direction));
    assign(RspCancelOrderFieldJson["OffsetFlag"], getOffsetFlagString(pRspCancelOrderField->OffsetFlag));
    assign(RspCancelOrderFieldJson["OrderLocalID"], pRspCancelOrderField->OrderLocalID);
    assign(RspCancelOrderFieldJson["OrderMaker"], getOrderMakerString(pRspCancelOrderField->OrderMaker));
    assign(RspCancelOrderFieldJson["OrderType"], getOrderTypeString(pRspCancelOrderField->OrderType));
    assign(RspCancelOrderFieldJson["LandTime"], pRspCancelOrderField->LandTime);
    assign(RspCancelOrderFieldJson["SendTime"], pRspCancelOrderField->SendTime);
    assign(RspCancelOrderFieldJson["StrategyOrderID"], pRspCancelOrderField->StrategyOrderID);
    assign(RspCancelOrderFieldJson["OrderMode"], getOrderModeString(pRspCancelOrderField->OrderMode));
    assign(RspCancelOrderFieldJson["AssetType"], getAssetTypeString(pRspCancelOrderField->AssetType));
    assign(RspCancelOrderFieldJson["TradeChannel"], pRspCancelOrderField->TradeChannel);
    assign(RspCancelOrderFieldJson["OrderXO"], getOrderXOString(pRspCancelOrderField->OrderXO));
    assign(RspCancelOrderFieldJson["PlatformTime"], pRspCancelOrderField->PlatformTime);
    assign(RspCancelOrderFieldJson["OrderSysID"], pRspCancelOrderField->OrderSysID);
    assign(RspCancelOrderFieldJson["OrderForeID"], pRspCancelOrderField->OrderForeID);
    assign(RspCancelOrderFieldJson["CreateTime"], pRspCancelOrderField->CreateTime);
    assign(RspCancelOrderFieldJson["ModifyTime"], pRspCancelOrderField->ModifyTime);
    assign(RspCancelOrderFieldJson["RspLocalTime"], pRspCancelOrderField->RspLocalTime);
    assign(RspCancelOrderFieldJson["Cost"], pRspCancelOrderField->Cost);
    assign(RspCancelOrderFieldJson["TradePrice"], pRspCancelOrderField->TradePrice);
    assign(RspCancelOrderFieldJson["TradeVolume"], pRspCancelOrderField->TradeVolume);
    assign(RspCancelOrderFieldJson["TradeValue"], pRspCancelOrderField->TradeValue);
    assign(RspCancelOrderFieldJson["OrderStatus"], getOrderStatusString(pRspCancelOrderField->OrderStatus));
    assign(RspCancelOrderFieldJson["SessionID"], pRspCancelOrderField->SessionID);
    assign(RspCancelOrderFieldJson["RequestID"], pRspCancelOrderField->RequestID);
    assign(RspCancelOrderFieldJson["RequestForeID"], pRspCancelOrderField->RequestForeID);
    assign(RspCancelOrderFieldJson["Fee"], pRspCancelOrderField->Fee);
    assign(RspCancelOrderFieldJson["FeeCurrency"], pRspCancelOrderField->FeeCurrency);
    assign(RspCancelOrderFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspCancelOrderFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleSubPositionField(nlohmann::json& SubPositionFieldJson, const CUTSubPositionField* pSubPositionField)
{
    assign(SubPositionFieldJson["ExchangeID"], pSubPositionField->ExchangeID);
    assign(SubPositionFieldJson["InstrumentID"], pSubPositionField->InstrumentID);
    assign(SubPositionFieldJson["AssetType"], getAssetTypeString(pSubPositionField->AssetType));
    assign(SubPositionFieldJson["PosiDirection"], getPosiDirectionString(pSubPositionField->PosiDirection));
    assign(SubPositionFieldJson["TradeChannel"], pSubPositionField->TradeChannel);
}
void RedisPublisher::handleRtnPositionField(nlohmann::json& RtnPositionFieldJson, const CUTRtnPositionField* pRtnPositionField)
{
    assign(RtnPositionFieldJson["ExchangeID"], pRtnPositionField->ExchangeID);
    assign(RtnPositionFieldJson["AccountName"], pRtnPositionField->AccountName);
    assign(RtnPositionFieldJson["AccountType"], getAccountTypeString(pRtnPositionField->AccountType));
    assign(RtnPositionFieldJson["InstrumentID"], pRtnPositionField->InstrumentID);
    assign(RtnPositionFieldJson["PosiDirection"], getPosiDirectionString(pRtnPositionField->PosiDirection));
    assign(RtnPositionFieldJson["Position"], pRtnPositionField->Position);
    assign(RtnPositionFieldJson["YDPosition"], pRtnPositionField->YDPosition);
    assign(RtnPositionFieldJson["Price"], pRtnPositionField->Price);
    assign(RtnPositionFieldJson["Frozen"], pRtnPositionField->Frozen);
    assign(RtnPositionFieldJson["Available"], pRtnPositionField->Available);
    assign(RtnPositionFieldJson["TotalAvail"], pRtnPositionField->TotalAvail);
    assign(RtnPositionFieldJson["UpdateTime"], pRtnPositionField->UpdateTime);
    assign(RtnPositionFieldJson["CreateTime"], pRtnPositionField->CreateTime);
    assign(RtnPositionFieldJson["CurrencyID"], pRtnPositionField->CurrencyID);
    assign(RtnPositionFieldJson["BaseCurrency"], pRtnPositionField->BaseCurrency);
    assign(RtnPositionFieldJson["OrderMargin"], pRtnPositionField->OrderMargin);
    assign(RtnPositionFieldJson["PositionMargin"], pRtnPositionField->PositionMargin);
    assign(RtnPositionFieldJson["FrozenBuy"], pRtnPositionField->FrozenBuy);
    assign(RtnPositionFieldJson["FrozenSell"], pRtnPositionField->FrozenSell);
    assign(RtnPositionFieldJson["PositionID"], pRtnPositionField->PositionID);
    assign(RtnPositionFieldJson["AssetType"], getAssetTypeString(pRtnPositionField->AssetType));
    assign(RtnPositionFieldJson["TradeChannel"], pRtnPositionField->TradeChannel);
}
void RedisPublisher::handleReqQryOrderField(nlohmann::json& ReqQryOrderFieldJson, const CUTReqQryOrderField* pReqQryOrderField)
{
    assign(ReqQryOrderFieldJson["StrategyOrderID"], pReqQryOrderField->StrategyOrderID);
    assign(ReqQryOrderFieldJson["OrderSysID"], pReqQryOrderField->OrderSysID);
    assign(ReqQryOrderFieldJson["ExchangeID"], pReqQryOrderField->ExchangeID);
    assign(ReqQryOrderFieldJson["InstrumentID"], pReqQryOrderField->InstrumentID);
    assign(ReqQryOrderFieldJson["TradeChannel"], pReqQryOrderField->TradeChannel);
    assign(ReqQryOrderFieldJson["TimeStart"], pReqQryOrderField->TimeStart);
    assign(ReqQryOrderFieldJson["TimeEnd"], pReqQryOrderField->TimeEnd);
}
void RedisPublisher::handleRspQryOrderField(nlohmann::json& RspQryOrderFieldJson, const CUTRspQryOrderField* pRspQryOrderField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryOrderFieldJson["ExchangeID"], pRspQryOrderField->ExchangeID);
    assign(RspQryOrderFieldJson["InstrumentID"], pRspQryOrderField->InstrumentID);
    assign(RspQryOrderFieldJson["Price"], pRspQryOrderField->Price);
    assign(RspQryOrderFieldJson["Volume"], pRspQryOrderField->Volume);
    assign(RspQryOrderFieldJson["Direction"], getDirectionString(pRspQryOrderField->Direction));
    assign(RspQryOrderFieldJson["OffsetFlag"], getOffsetFlagString(pRspQryOrderField->OffsetFlag));
    assign(RspQryOrderFieldJson["OrderLocalID"], pRspQryOrderField->OrderLocalID);
    assign(RspQryOrderFieldJson["OrderMaker"], getOrderMakerString(pRspQryOrderField->OrderMaker));
    assign(RspQryOrderFieldJson["OrderType"], getOrderTypeString(pRspQryOrderField->OrderType));
    assign(RspQryOrderFieldJson["LandTime"], pRspQryOrderField->LandTime);
    assign(RspQryOrderFieldJson["SendTime"], pRspQryOrderField->SendTime);
    assign(RspQryOrderFieldJson["StrategyOrderID"], pRspQryOrderField->StrategyOrderID);
    assign(RspQryOrderFieldJson["OrderMode"], getOrderModeString(pRspQryOrderField->OrderMode));
    assign(RspQryOrderFieldJson["AssetType"], getAssetTypeString(pRspQryOrderField->AssetType));
    assign(RspQryOrderFieldJson["TradeChannel"], pRspQryOrderField->TradeChannel);
    assign(RspQryOrderFieldJson["OrderXO"], getOrderXOString(pRspQryOrderField->OrderXO));
    assign(RspQryOrderFieldJson["PlatformTime"], pRspQryOrderField->PlatformTime);
    assign(RspQryOrderFieldJson["OrderSysID"], pRspQryOrderField->OrderSysID);
    assign(RspQryOrderFieldJson["OrderForeID"], pRspQryOrderField->OrderForeID);
    assign(RspQryOrderFieldJson["CreateTime"], pRspQryOrderField->CreateTime);
    assign(RspQryOrderFieldJson["ModifyTime"], pRspQryOrderField->ModifyTime);
    assign(RspQryOrderFieldJson["RspLocalTime"], pRspQryOrderField->RspLocalTime);
    assign(RspQryOrderFieldJson["Cost"], pRspQryOrderField->Cost);
    assign(RspQryOrderFieldJson["TradePrice"], pRspQryOrderField->TradePrice);
    assign(RspQryOrderFieldJson["TradeVolume"], pRspQryOrderField->TradeVolume);
    assign(RspQryOrderFieldJson["TradeValue"], pRspQryOrderField->TradeValue);
    assign(RspQryOrderFieldJson["OrderStatus"], getOrderStatusString(pRspQryOrderField->OrderStatus));
    assign(RspQryOrderFieldJson["SessionID"], pRspQryOrderField->SessionID);
    assign(RspQryOrderFieldJson["RequestID"], pRspQryOrderField->RequestID);
    assign(RspQryOrderFieldJson["RequestForeID"], pRspQryOrderField->RequestForeID);
    assign(RspQryOrderFieldJson["Fee"], pRspQryOrderField->Fee);
    assign(RspQryOrderFieldJson["FeeCurrency"], pRspQryOrderField->FeeCurrency);
    assign(RspQryOrderFieldJson["TimeStart"], pRspQryOrderField->TimeStart);
    assign(RspQryOrderFieldJson["TimeEnd"], pRspQryOrderField->TimeEnd);
    assign(RspQryOrderFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryOrderFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleReqQryTradeField(nlohmann::json& ReqQryTradeFieldJson, const CUTReqQryTradeField* pReqQryTradeField)
{
    assign(ReqQryTradeFieldJson["TradeID"], pReqQryTradeField->TradeID);
    assign(ReqQryTradeFieldJson["TradeChannel"], pReqQryTradeField->TradeChannel);
}
void RedisPublisher::handleRspQryTradeField(nlohmann::json& RspQryTradeFieldJson, const CUTRspQryTradeField* pRspQryTradeField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryTradeFieldJson["TradeID"], pRspQryTradeField->TradeID);
    assign(RspQryTradeFieldJson["OrderSysID"], pRspQryTradeField->OrderSysID);
    assign(RspQryTradeFieldJson["ExchangeID"], pRspQryTradeField->ExchangeID);
    assign(RspQryTradeFieldJson["InstrumentID"], pRspQryTradeField->InstrumentID);
    assign(RspQryTradeFieldJson["MatchPrice"], pRspQryTradeField->MatchPrice);
    assign(RspQryTradeFieldJson["MatchVolume"], pRspQryTradeField->MatchVolume);
    assign(RspQryTradeFieldJson["MatchValue"], pRspQryTradeField->MatchValue);
    assign(RspQryTradeFieldJson["Direction"], getDirectionString(pRspQryTradeField->Direction));
    assign(RspQryTradeFieldJson["OrderLocalID"], pRspQryTradeField->OrderLocalID);
    assign(RspQryTradeFieldJson["Fee"], pRspQryTradeField->Fee);
    assign(RspQryTradeFieldJson["FeeCurrency"], pRspQryTradeField->FeeCurrency);
    assign(RspQryTradeFieldJson["PlatformTime"], pRspQryTradeField->PlatformTime);
    assign(RspQryTradeFieldJson["TradeTime"], pRspQryTradeField->TradeTime);
    assign(RspQryTradeFieldJson["RspLocalTime"], pRspQryTradeField->RspLocalTime);
    assign(RspQryTradeFieldJson["Price"], pRspQryTradeField->Price);
    assign(RspQryTradeFieldJson["StrategyOrderID"], pRspQryTradeField->StrategyOrderID);
    assign(RspQryTradeFieldJson["OrderMaker"], getOrderMakerString(pRspQryTradeField->OrderMaker));
    assign(RspQryTradeFieldJson["AssetType"], getAssetTypeString(pRspQryTradeField->AssetType));
    assign(RspQryTradeFieldJson["TradeChannel"], pRspQryTradeField->TradeChannel);
    assign(RspQryTradeFieldJson["SessionID"], pRspQryTradeField->SessionID);
    assign(RspQryTradeFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryTradeFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleReqQryAccountField(nlohmann::json& ReqQryAccountFieldJson, const CUTReqQryAccountField* pReqQryAccountField)
{
    assign(ReqQryAccountFieldJson["Currency"], pReqQryAccountField->Currency);
    assign(ReqQryAccountFieldJson["TradeChannel"], pReqQryAccountField->TradeChannel);
}
void RedisPublisher::handleRspQryAccountField(nlohmann::json& RspQryAccountFieldJson, const CUTRspQryAccountField* pRspQryAccountField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryAccountFieldJson["QueryTime"], pRspQryAccountField->QueryTime);
    assign(RspQryAccountFieldJson["ExchangeID"], pRspQryAccountField->ExchangeID);
    assign(RspQryAccountFieldJson["Currency"], pRspQryAccountField->Currency);
    assign(RspQryAccountFieldJson["PositionMargin"], pRspQryAccountField->PositionMargin);
    assign(RspQryAccountFieldJson["OrderMargin"], pRspQryAccountField->OrderMargin);
    assign(RspQryAccountFieldJson["Available"], pRspQryAccountField->Available);
    assign(RspQryAccountFieldJson["PositionBalance"], pRspQryAccountField->PositionBalance);
    assign(RspQryAccountFieldJson["TotalAsset"], pRspQryAccountField->TotalAsset);
    assign(RspQryAccountFieldJson["TradeChannel"], pRspQryAccountField->TradeChannel);
    assign(RspQryAccountFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryAccountFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleSubAccountField(nlohmann::json& SubAccountFieldJson, const CUTSubAccountField* pSubAccountField)
{
    assign(SubAccountFieldJson["Currency"], pSubAccountField->Currency);
    assign(SubAccountFieldJson["AssetType"], getAssetTypeString(pSubAccountField->AssetType));
    assign(SubAccountFieldJson["TradeChannel"], pSubAccountField->TradeChannel);
}
void RedisPublisher::handleRtnAccountField(nlohmann::json& RtnAccountFieldJson, const CUTRtnAccountField* pRtnAccountField)
{
    assign(RtnAccountFieldJson["ExchangeID"], pRtnAccountField->ExchangeID);
    assign(RtnAccountFieldJson["AccountName"], pRtnAccountField->AccountName);
    assign(RtnAccountFieldJson["AccountType"], getAccountTypeString(pRtnAccountField->AccountType));
    assign(RtnAccountFieldJson["CurrencyName"], pRtnAccountField->CurrencyName);
    assign(RtnAccountFieldJson["CurrencyQuantity"], pRtnAccountField->CurrencyQuantity);
    assign(RtnAccountFieldJson["PositionMargin"], pRtnAccountField->PositionMargin);
    assign(RtnAccountFieldJson["OrderMargin"], pRtnAccountField->OrderMargin);
    assign(RtnAccountFieldJson["PositionBalance"], pRtnAccountField->PositionBalance);
    assign(RtnAccountFieldJson["TotalBalance"], pRtnAccountField->TotalBalance);
    assign(RtnAccountFieldJson["Available"], pRtnAccountField->Available);
    assign(RtnAccountFieldJson["LongAvailable"], pRtnAccountField->LongAvailable);
    assign(RtnAccountFieldJson["ShortAvailable"], pRtnAccountField->ShortAvailable);
    assign(RtnAccountFieldJson["ActualLongAvail"], pRtnAccountField->ActualLongAvail);
    assign(RtnAccountFieldJson["ActualShortAvail"], pRtnAccountField->ActualShortAvail);
    assign(RtnAccountFieldJson["Frozen"], pRtnAccountField->Frozen);
    assign(RtnAccountFieldJson["Fee"], pRtnAccountField->Fee);
    assign(RtnAccountFieldJson["FrozenBuy"], pRtnAccountField->FrozenBuy);
    assign(RtnAccountFieldJson["FrozenSell"], pRtnAccountField->FrozenSell);
    assign(RtnAccountFieldJson["UpdateTime"], pRtnAccountField->UpdateTime);
    assign(RtnAccountFieldJson["CurrencyID"], pRtnAccountField->CurrencyID);
    assign(RtnAccountFieldJson["AssetType"], getAssetTypeString(pRtnAccountField->AssetType));
    assign(RtnAccountFieldJson["TradeChannel"], pRtnAccountField->TradeChannel);
    assign(RtnAccountFieldJson["Borrow"], pRtnAccountField->Borrow);
    assign(RtnAccountFieldJson["Lend"], pRtnAccountField->Lend);
    assign(RtnAccountFieldJson["DebtOffset"], pRtnAccountField->DebtOffset);
    assign(RtnAccountFieldJson["TransferOffset"], pRtnAccountField->TransferOffset);
}
void RedisPublisher::handleReqLoginField(nlohmann::json& ReqLoginFieldJson, const CUTReqLoginField* pReqLoginField)
{
    assign(ReqLoginFieldJson["ClientType"], getClientTypeString(pReqLoginField->ClientType));
    assign(ReqLoginFieldJson["UserName"], pReqLoginField->UserName);
    assign(ReqLoginFieldJson["ClientName"], pReqLoginField->ClientName);
    assign(ReqLoginFieldJson["Password"], pReqLoginField->Password);
    assign(ReqLoginFieldJson["ReqSequenceID"], pReqLoginField->ReqSequenceID);
    assign(ReqLoginFieldJson["ApiKey"], pReqLoginField->ApiKey);
    assign(ReqLoginFieldJson["ApiSecret"], pReqLoginField->ApiSecret);
    assign(ReqLoginFieldJson["ApiPassword"], pReqLoginField->ApiPassword);
}
void RedisPublisher::handleRspLoginField(nlohmann::json& RspLoginFieldJson, const CUTRspLoginField* pRspLoginField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspLoginFieldJson["ClientType"], getClientTypeString(pRspLoginField->ClientType));
    assign(RspLoginFieldJson["UserName"], pRspLoginField->UserName);
    assign(RspLoginFieldJson["Password"], pRspLoginField->Password);
    assign(RspLoginFieldJson["ServerTime"], pRspLoginField->ServerTime);
    assign(RspLoginFieldJson["TimeB4Launch"], pRspLoginField->TimeB4Launch);
    assign(RspLoginFieldJson["RspSequenceID"], pRspLoginField->RspSequenceID);
    assign(RspLoginFieldJson["StrategyName"], pRspLoginField->StrategyName);
    assign(RspLoginFieldJson["AccessToken"], pRspLoginField->AccessToken);
    assign(RspLoginFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspLoginFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleReqLogoutField(nlohmann::json& ReqLogoutFieldJson, const CUTReqLogoutField* pReqLogoutField)
{
    assign(ReqLogoutFieldJson["UserName"], pReqLogoutField->UserName);
    assign(ReqLogoutFieldJson["AccountName"], pReqLogoutField->AccountName);
}
void RedisPublisher::handleRspLogoutField(nlohmann::json& RspLogoutFieldJson, const CUTRspLogoutField* pRspLogoutField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspLogoutFieldJson["UserName"], pRspLogoutField->UserName);
    assign(RspLogoutFieldJson["ServerTime"], pRspLogoutField->ServerTime);
    assign(RspLogoutFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspLogoutFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleReqQryPositionField(nlohmann::json& ReqQryPositionFieldJson, const CUTReqQryPositionField* pReqQryPositionField)
{
    assign(ReqQryPositionFieldJson["ExchangeID"], pReqQryPositionField->ExchangeID);
    assign(ReqQryPositionFieldJson["InstrumentID"], pReqQryPositionField->InstrumentID);
    assign(ReqQryPositionFieldJson["AccessToken"], pReqQryPositionField->AccessToken);
    assign(ReqQryPositionFieldJson["TradeChannel"], pReqQryPositionField->TradeChannel);
}
void RedisPublisher::handleRspQryPositionField(nlohmann::json& RspQryPositionFieldJson, const CUTRspQryPositionField* pRspQryPositionField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryPositionFieldJson["ExchangeID"], pRspQryPositionField->ExchangeID);
    assign(RspQryPositionFieldJson["AccountName"], pRspQryPositionField->AccountName);
    assign(RspQryPositionFieldJson["AccountType"], getAccountTypeString(pRspQryPositionField->AccountType));
    assign(RspQryPositionFieldJson["InstrumentID"], pRspQryPositionField->InstrumentID);
    assign(RspQryPositionFieldJson["PosiDirection"], getPosiDirectionString(pRspQryPositionField->PosiDirection));
    assign(RspQryPositionFieldJson["Position"], pRspQryPositionField->Position);
    assign(RspQryPositionFieldJson["YDPosition"], pRspQryPositionField->YDPosition);
    assign(RspQryPositionFieldJson["Price"], pRspQryPositionField->Price);
    assign(RspQryPositionFieldJson["Frozen"], pRspQryPositionField->Frozen);
    assign(RspQryPositionFieldJson["Available"], pRspQryPositionField->Available);
    assign(RspQryPositionFieldJson["TotalAvail"], pRspQryPositionField->TotalAvail);
    assign(RspQryPositionFieldJson["UpdateTime"], pRspQryPositionField->UpdateTime);
    assign(RspQryPositionFieldJson["CreateTime"], pRspQryPositionField->CreateTime);
    assign(RspQryPositionFieldJson["CurrencyID"], pRspQryPositionField->CurrencyID);
    assign(RspQryPositionFieldJson["BaseCurrency"], pRspQryPositionField->BaseCurrency);
    assign(RspQryPositionFieldJson["OrderMargin"], pRspQryPositionField->OrderMargin);
    assign(RspQryPositionFieldJson["PositionMargin"], pRspQryPositionField->PositionMargin);
    assign(RspQryPositionFieldJson["FrozenBuy"], pRspQryPositionField->FrozenBuy);
    assign(RspQryPositionFieldJson["FrozenSell"], pRspQryPositionField->FrozenSell);
    assign(RspQryPositionFieldJson["PositionID"], pRspQryPositionField->PositionID);
    assign(RspQryPositionFieldJson["AssetType"], getAssetTypeString(pRspQryPositionField->AssetType));
    assign(RspQryPositionFieldJson["TradeChannel"], pRspQryPositionField->TradeChannel);
    assign(RspQryPositionFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryPositionFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleRtnPlatformDetailField(nlohmann::json& RtnPlatformDetailFieldJson, const CUTRtnPlatformDetailField* pRtnPlatformDetailField)
{
    assign(RtnPlatformDetailFieldJson["Active"], getActiveString(pRtnPlatformDetailField->Active));
    assign(RtnPlatformDetailFieldJson["UpdateTime"], pRtnPlatformDetailField->UpdateTime);
}
void RedisPublisher::handleRtnStrategyDetailField(nlohmann::json& RtnStrategyDetailFieldJson, const CUTRtnStrategyDetailField* pRtnStrategyDetailField)
{
    assign(RtnStrategyDetailFieldJson["Active"], getActiveString(pRtnStrategyDetailField->Active));
    assign(RtnStrategyDetailFieldJson["StrategyName"], pRtnStrategyDetailField->StrategyName);
    assign(RtnStrategyDetailFieldJson["UpdateTime"], pRtnStrategyDetailField->UpdateTime);
}
void RedisPublisher::handleRtnBusinessDebtField(nlohmann::json& RtnBusinessDebtFieldJson, const CUTRtnBusinessDebtField* pRtnBusinessDebtField)
{
    assign(RtnBusinessDebtFieldJson["ExchangeID"], pRtnBusinessDebtField->ExchangeID);
    assign(RtnBusinessDebtFieldJson["AccountName"], pRtnBusinessDebtField->AccountName);
    assign(RtnBusinessDebtFieldJson["AccountType"], getAccountTypeString(pRtnBusinessDebtField->AccountType));
    assign(RtnBusinessDebtFieldJson["CurrencyName"], pRtnBusinessDebtField->CurrencyName);
    assign(RtnBusinessDebtFieldJson["CurrBusinessName"], pRtnBusinessDebtField->CurrBusinessName);
    assign(RtnBusinessDebtFieldJson["DebtBusinessName"], pRtnBusinessDebtField->DebtBusinessName);
    assign(RtnBusinessDebtFieldJson["DebtDirection"], getDebtDirectionString(pRtnBusinessDebtField->DebtDirection));
    assign(RtnBusinessDebtFieldJson["DebtAmount"], pRtnBusinessDebtField->DebtAmount);
    assign(RtnBusinessDebtFieldJson["CreateTime"], pRtnBusinessDebtField->CreateTime);
}
void RedisPublisher::handleReqQryAccountBusinessField(nlohmann::json& ReqQryAccountBusinessFieldJson, const CUTReqQryAccountBusinessField* pReqQryAccountBusinessField)
{
    assign(ReqQryAccountBusinessFieldJson["Currency"], pReqQryAccountBusinessField->Currency);
    assign(ReqQryAccountBusinessFieldJson["TradeChannel"], pReqQryAccountBusinessField->TradeChannel);
}
void RedisPublisher::handleRspQryAccountBusinessField(nlohmann::json& RspQryAccountBusinessFieldJson, const CUTRspQryAccountBusinessField* pRspQryAccountBusinessField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryAccountBusinessFieldJson["QueryTime"], pRspQryAccountBusinessField->QueryTime);
    assign(RspQryAccountBusinessFieldJson["ExchangeID"], pRspQryAccountBusinessField->ExchangeID);
    assign(RspQryAccountBusinessFieldJson["Currency"], pRspQryAccountBusinessField->Currency);
    assign(RspQryAccountBusinessFieldJson["PositionMargin"], pRspQryAccountBusinessField->PositionMargin);
    assign(RspQryAccountBusinessFieldJson["OrderMargin"], pRspQryAccountBusinessField->OrderMargin);
    assign(RspQryAccountBusinessFieldJson["Available"], pRspQryAccountBusinessField->Available);
    assign(RspQryAccountBusinessFieldJson["PositionBalance"], pRspQryAccountBusinessField->PositionBalance);
    assign(RspQryAccountBusinessFieldJson["TotalAsset"], pRspQryAccountBusinessField->TotalAsset);
    assign(RspQryAccountBusinessFieldJson["TradeChannel"], pRspQryAccountBusinessField->TradeChannel);
    assign(RspQryAccountBusinessFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryAccountBusinessFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleRtnAccountBusinessField(nlohmann::json& RtnAccountBusinessFieldJson, const CUTRtnAccountBusinessField* pRtnAccountBusinessField)
{
    assign(RtnAccountBusinessFieldJson["ExchangeID"], pRtnAccountBusinessField->ExchangeID);
    assign(RtnAccountBusinessFieldJson["AccountName"], pRtnAccountBusinessField->AccountName);
    assign(RtnAccountBusinessFieldJson["AccountType"], getAccountTypeString(pRtnAccountBusinessField->AccountType));
    assign(RtnAccountBusinessFieldJson["CurrencyName"], pRtnAccountBusinessField->CurrencyName);
    assign(RtnAccountBusinessFieldJson["CurrencyQuantity"], pRtnAccountBusinessField->CurrencyQuantity);
    assign(RtnAccountBusinessFieldJson["PositionMargin"], pRtnAccountBusinessField->PositionMargin);
    assign(RtnAccountBusinessFieldJson["OrderMargin"], pRtnAccountBusinessField->OrderMargin);
    assign(RtnAccountBusinessFieldJson["PositionBalance"], pRtnAccountBusinessField->PositionBalance);
    assign(RtnAccountBusinessFieldJson["TotalBalance"], pRtnAccountBusinessField->TotalBalance);
    assign(RtnAccountBusinessFieldJson["Available"], pRtnAccountBusinessField->Available);
    assign(RtnAccountBusinessFieldJson["LongAvailable"], pRtnAccountBusinessField->LongAvailable);
    assign(RtnAccountBusinessFieldJson["ShortAvailable"], pRtnAccountBusinessField->ShortAvailable);
    assign(RtnAccountBusinessFieldJson["ActualLongAvail"], pRtnAccountBusinessField->ActualLongAvail);
    assign(RtnAccountBusinessFieldJson["ActualShortAvail"], pRtnAccountBusinessField->ActualShortAvail);
    assign(RtnAccountBusinessFieldJson["Frozen"], pRtnAccountBusinessField->Frozen);
    assign(RtnAccountBusinessFieldJson["Fee"], pRtnAccountBusinessField->Fee);
    assign(RtnAccountBusinessFieldJson["FrozenBuy"], pRtnAccountBusinessField->FrozenBuy);
    assign(RtnAccountBusinessFieldJson["FrozenSell"], pRtnAccountBusinessField->FrozenSell);
    assign(RtnAccountBusinessFieldJson["UpdateTime"], pRtnAccountBusinessField->UpdateTime);
    assign(RtnAccountBusinessFieldJson["CurrencyID"], pRtnAccountBusinessField->CurrencyID);
    assign(RtnAccountBusinessFieldJson["AssetType"], getAssetTypeString(pRtnAccountBusinessField->AssetType));
    assign(RtnAccountBusinessFieldJson["TradeChannel"], pRtnAccountBusinessField->TradeChannel);
    assign(RtnAccountBusinessFieldJson["Borrow"], pRtnAccountBusinessField->Borrow);
    assign(RtnAccountBusinessFieldJson["Lend"], pRtnAccountBusinessField->Lend);
    assign(RtnAccountBusinessFieldJson["DebtOffset"], pRtnAccountBusinessField->DebtOffset);
    assign(RtnAccountBusinessFieldJson["TransferOffset"], pRtnAccountBusinessField->TransferOffset);
}
void RedisPublisher::handleReqManualTransactField(nlohmann::json& ReqManualTransactFieldJson, const CUTReqManualTransactField* pReqManualTransactField)
{
    assign(ReqManualTransactFieldJson["EventID"], pReqManualTransactField->EventID);
    assign(ReqManualTransactFieldJson["ExchangeID"], pReqManualTransactField->ExchangeID);
    assign(ReqManualTransactFieldJson["AccountName"], pReqManualTransactField->AccountName);
    assign(ReqManualTransactFieldJson["BusinessName"], pReqManualTransactField->BusinessName);
    assign(ReqManualTransactFieldJson["AccountType"], getAccountTypeString(pReqManualTransactField->AccountType));
    assign(ReqManualTransactFieldJson["TransactDirection"], getTransactDirectionString(pReqManualTransactField->TransactDirection));
    assign(ReqManualTransactFieldJson["Currency"], pReqManualTransactField->Currency);
    assign(ReqManualTransactFieldJson["Amount"], pReqManualTransactField->Amount);
    assign(ReqManualTransactFieldJson["CreateTime"], pReqManualTransactField->CreateTime);
}
void RedisPublisher::handleReqTransactField(nlohmann::json& ReqTransactFieldJson, const CUTReqTransactField* pReqTransactField)
{
    assign(ReqTransactFieldJson["EventID"], pReqTransactField->EventID);
    assign(ReqTransactFieldJson["AccountNameFrom"], pReqTransactField->AccountNameFrom);
    assign(ReqTransactFieldJson["AccountNameTo"], pReqTransactField->AccountNameTo);
    assign(ReqTransactFieldJson["Currency"], pReqTransactField->Currency);
    assign(ReqTransactFieldJson["Direction"], getTransactDirectionString(pReqTransactField->Direction));
    assign(ReqTransactFieldJson["Amount"], pReqTransactField->Amount);
    assign(ReqTransactFieldJson["TradeChannelFrom"], pReqTransactField->TradeChannelFrom);
    assign(ReqTransactFieldJson["TradeChannelTo"], pReqTransactField->TradeChannelTo);
    assign(ReqTransactFieldJson["Type"], getTransactTypeString(pReqTransactField->Type));
    assign(ReqTransactFieldJson["Address"], pReqTransactField->Address);
    assign(ReqTransactFieldJson["AddMemo"], pReqTransactField->AddMemo);
}
void RedisPublisher::handleRspTransactField(nlohmann::json& RspTransactFieldJson, const CUTRspTransactField* pRspTransactField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspTransactFieldJson["EventID"], pRspTransactField->EventID);
    assign(RspTransactFieldJson["AccountNameFrom"], pRspTransactField->AccountNameFrom);
    assign(RspTransactFieldJson["AccountNameTo"], pRspTransactField->AccountNameTo);
    assign(RspTransactFieldJson["Currency"], pRspTransactField->Currency);
    assign(RspTransactFieldJson["Direction"], getTransactDirectionString(pRspTransactField->Direction));
    assign(RspTransactFieldJson["Amount"], pRspTransactField->Amount);
    assign(RspTransactFieldJson["TradeChannelFrom"], pRspTransactField->TradeChannelFrom);
    assign(RspTransactFieldJson["TradeChannelTo"], pRspTransactField->TradeChannelTo);
    assign(RspTransactFieldJson["Type"], getTransactTypeString(pRspTransactField->Type));
    assign(RspTransactFieldJson["Address"], pRspTransactField->Address);
    assign(RspTransactFieldJson["AddMemo"], pRspTransactField->AddMemo);
    assign(RspTransactFieldJson["ID"], pRspTransactField->ID);
    assign(RspTransactFieldJson["CreateTime"], pRspTransactField->CreateTime);
    assign(RspTransactFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspTransactFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleRtnTransactField(nlohmann::json& RtnTransactFieldJson, const CUTRtnTransactField* pRtnTransactField)
{
    assign(RtnTransactFieldJson["EventID"], pRtnTransactField->EventID);
    assign(RtnTransactFieldJson["AccountNameFrom"], pRtnTransactField->AccountNameFrom);
    assign(RtnTransactFieldJson["AccountNameTo"], pRtnTransactField->AccountNameTo);
    assign(RtnTransactFieldJson["Currency"], pRtnTransactField->Currency);
    assign(RtnTransactFieldJson["Direction"], getTransactDirectionString(pRtnTransactField->Direction));
    assign(RtnTransactFieldJson["Amount"], pRtnTransactField->Amount);
    assign(RtnTransactFieldJson["TradeChannelFrom"], pRtnTransactField->TradeChannelFrom);
    assign(RtnTransactFieldJson["TradeChannelTo"], pRtnTransactField->TradeChannelTo);
    assign(RtnTransactFieldJson["Type"], getTransactTypeString(pRtnTransactField->Type));
    assign(RtnTransactFieldJson["Address"], pRtnTransactField->Address);
    assign(RtnTransactFieldJson["AddMemo"], pRtnTransactField->AddMemo);
    assign(RtnTransactFieldJson["ID"], pRtnTransactField->ID);
    assign(RtnTransactFieldJson["SessionID"], pRtnTransactField->SessionID);
    assign(RtnTransactFieldJson["TransactStatus"], getTransactStatusString(pRtnTransactField->TransactStatus));
    assign(RtnTransactFieldJson["Fee"], pRtnTransactField->Fee);
    assign(RtnTransactFieldJson["FeeCurrency"], pRtnTransactField->FeeCurrency);
    assign(RtnTransactFieldJson["CreateTime"], pRtnTransactField->CreateTime);
}
void RedisPublisher::handleReqQryTransactField(nlohmann::json& ReqQryTransactFieldJson, const CUTReqQryTransactField* pReqQryTransactField)
{
    assign(ReqQryTransactFieldJson["EventID"], pReqQryTransactField->EventID);
    assign(ReqQryTransactFieldJson["AccountNameFrom"], pReqQryTransactField->AccountNameFrom);
    assign(ReqQryTransactFieldJson["AccountNameTo"], pReqQryTransactField->AccountNameTo);
    assign(ReqQryTransactFieldJson["Direction"], getTransactDirectionString(pReqQryTransactField->Direction));
    assign(ReqQryTransactFieldJson["Currency"], pReqQryTransactField->Currency);
    assign(ReqQryTransactFieldJson["TradeChannelFrom"], pReqQryTransactField->TradeChannelFrom);
    assign(ReqQryTransactFieldJson["TradeChannelTo"], pReqQryTransactField->TradeChannelTo);
}
void RedisPublisher::handleRspQryTransactField(nlohmann::json& RspQryTransactFieldJson, const CUTRspQryTransactField* pRspQryTransactField, const CUTRspInfoField* pRspInfoField)
{
    assign(RspQryTransactFieldJson["EventID"], pRspQryTransactField->EventID);
    assign(RspQryTransactFieldJson["AccountNameFrom"], pRspQryTransactField->AccountNameFrom);
    assign(RspQryTransactFieldJson["AccountNameTo"], pRspQryTransactField->AccountNameTo);
    assign(RspQryTransactFieldJson["Currency"], pRspQryTransactField->Currency);
    assign(RspQryTransactFieldJson["Direction"], getTransactDirectionString(pRspQryTransactField->Direction));
    assign(RspQryTransactFieldJson["Amount"], pRspQryTransactField->Amount);
    assign(RspQryTransactFieldJson["TradeChannelFrom"], pRspQryTransactField->TradeChannelFrom);
    assign(RspQryTransactFieldJson["TradeChannelTo"], pRspQryTransactField->TradeChannelTo);
    assign(RspQryTransactFieldJson["Type"], getTransactTypeString(pRspQryTransactField->Type));
    assign(RspQryTransactFieldJson["Address"], pRspQryTransactField->Address);
    assign(RspQryTransactFieldJson["AddMemo"], pRspQryTransactField->AddMemo);
    assign(RspQryTransactFieldJson["ID"], pRspQryTransactField->ID);
    assign(RspQryTransactFieldJson["SessionID"], pRspQryTransactField->SessionID);
    assign(RspQryTransactFieldJson["TransactStatus"], getTransactStatusString(pRspQryTransactField->TransactStatus));
    assign(RspQryTransactFieldJson["Fee"], pRspQryTransactField->Fee);
    assign(RspQryTransactFieldJson["FeeCurrency"], pRspQryTransactField->FeeCurrency);
    assign(RspQryTransactFieldJson["CreateTime"], pRspQryTransactField->CreateTime);
    assign(RspQryTransactFieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(RspQryTransactFieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
void RedisPublisher::handleRtnDepthField(nlohmann::json& RtnDepthFieldJson, const CUTRtnDepthField* pRtnDepthField)
{
    assign(RtnDepthFieldJson["TradingDay"], pRtnDepthField->TradingDay);
    assign(RtnDepthFieldJson["ExchangeID"], pRtnDepthField->ExchangeID);
    assign(RtnDepthFieldJson["InstrumentID"], pRtnDepthField->InstrumentID);
    assign(RtnDepthFieldJson["ExchangeTime"], pRtnDepthField->ExchangeTime);
    assign(RtnDepthFieldJson["LocalTime"], pRtnDepthField->LocalTime);
    assign(RtnDepthFieldJson["ArriveTime"], pRtnDepthField->ArriveTime);
    assign(RtnDepthFieldJson["PlatformTime"], pRtnDepthField->PlatformTime);
    assign(RtnDepthFieldJson["AskPrice1"], pRtnDepthField->AskPrice1);
    assign(RtnDepthFieldJson["AskVolume1"], pRtnDepthField->AskVolume1);
    assign(RtnDepthFieldJson["BidPrice1"], pRtnDepthField->BidPrice1);
    assign(RtnDepthFieldJson["BidVolume1"], pRtnDepthField->BidVolume1);
    assign(RtnDepthFieldJson["AskPrice2"], pRtnDepthField->AskPrice2);
    assign(RtnDepthFieldJson["AskVolume2"], pRtnDepthField->AskVolume2);
    assign(RtnDepthFieldJson["BidPrice2"], pRtnDepthField->BidPrice2);
    assign(RtnDepthFieldJson["BidVolume2"], pRtnDepthField->BidVolume2);
    assign(RtnDepthFieldJson["AskPrice3"], pRtnDepthField->AskPrice3);
    assign(RtnDepthFieldJson["AskVolume3"], pRtnDepthField->AskVolume3);
    assign(RtnDepthFieldJson["BidPrice3"], pRtnDepthField->BidPrice3);
    assign(RtnDepthFieldJson["BidVolume3"], pRtnDepthField->BidVolume3);
    assign(RtnDepthFieldJson["AskPrice4"], pRtnDepthField->AskPrice4);
    assign(RtnDepthFieldJson["AskVolume4"], pRtnDepthField->AskVolume4);
    assign(RtnDepthFieldJson["BidPrice4"], pRtnDepthField->BidPrice4);
    assign(RtnDepthFieldJson["BidVolume4"], pRtnDepthField->BidVolume4);
    assign(RtnDepthFieldJson["AskPrice5"], pRtnDepthField->AskPrice5);
    assign(RtnDepthFieldJson["AskVolume5"], pRtnDepthField->AskVolume5);
    assign(RtnDepthFieldJson["BidPrice5"], pRtnDepthField->BidPrice5);
    assign(RtnDepthFieldJson["BidVolume5"], pRtnDepthField->BidVolume5);
}
void RedisPublisher::handleRtnL2DepthField(nlohmann::json& RtnL2DepthFieldJson, const CUTRtnL2DepthField* pRtnL2DepthField)
{
    assign(RtnL2DepthFieldJson["TradingDay"], pRtnL2DepthField->TradingDay);
    assign(RtnL2DepthFieldJson["ExchangeID"], pRtnL2DepthField->ExchangeID);
    assign(RtnL2DepthFieldJson["InstrumentID"], pRtnL2DepthField->InstrumentID);
    assign(RtnL2DepthFieldJson["ExchangeTime"], pRtnL2DepthField->ExchangeTime);
    assign(RtnL2DepthFieldJson["LocalTime"], pRtnL2DepthField->LocalTime);
    assign(RtnL2DepthFieldJson["ArriveTime"], pRtnL2DepthField->ArriveTime);
    assign(RtnL2DepthFieldJson["PlatformTime"], pRtnL2DepthField->PlatformTime);
    assign(RtnL2DepthFieldJson["AskPrice1"], pRtnL2DepthField->AskPrice1);
    assign(RtnL2DepthFieldJson["AskVolume1"], pRtnL2DepthField->AskVolume1);
    assign(RtnL2DepthFieldJson["BidPrice1"], pRtnL2DepthField->BidPrice1);
    assign(RtnL2DepthFieldJson["BidVolume1"], pRtnL2DepthField->BidVolume1);
    assign(RtnL2DepthFieldJson["AskPrice2"], pRtnL2DepthField->AskPrice2);
    assign(RtnL2DepthFieldJson["AskVolume2"], pRtnL2DepthField->AskVolume2);
    assign(RtnL2DepthFieldJson["BidPrice2"], pRtnL2DepthField->BidPrice2);
    assign(RtnL2DepthFieldJson["BidVolume2"], pRtnL2DepthField->BidVolume2);
    assign(RtnL2DepthFieldJson["AskPrice3"], pRtnL2DepthField->AskPrice3);
    assign(RtnL2DepthFieldJson["AskVolume3"], pRtnL2DepthField->AskVolume3);
    assign(RtnL2DepthFieldJson["BidPrice3"], pRtnL2DepthField->BidPrice3);
    assign(RtnL2DepthFieldJson["BidVolume3"], pRtnL2DepthField->BidVolume3);
    assign(RtnL2DepthFieldJson["AskPrice4"], pRtnL2DepthField->AskPrice4);
    assign(RtnL2DepthFieldJson["AskVolume4"], pRtnL2DepthField->AskVolume4);
    assign(RtnL2DepthFieldJson["BidPrice4"], pRtnL2DepthField->BidPrice4);
    assign(RtnL2DepthFieldJson["BidVolume4"], pRtnL2DepthField->BidVolume4);
    assign(RtnL2DepthFieldJson["AskPrice5"], pRtnL2DepthField->AskPrice5);
    assign(RtnL2DepthFieldJson["AskVolume5"], pRtnL2DepthField->AskVolume5);
    assign(RtnL2DepthFieldJson["BidPrice5"], pRtnL2DepthField->BidPrice5);
    assign(RtnL2DepthFieldJson["BidVolume5"], pRtnL2DepthField->BidVolume5);
    assign(RtnL2DepthFieldJson["AskPrice6"], pRtnL2DepthField->AskPrice6);
    assign(RtnL2DepthFieldJson["AskVolume6"], pRtnL2DepthField->AskVolume6);
    assign(RtnL2DepthFieldJson["BidPrice6"], pRtnL2DepthField->BidPrice6);
    assign(RtnL2DepthFieldJson["BidVolume6"], pRtnL2DepthField->BidVolume6);
    assign(RtnL2DepthFieldJson["AskPrice7"], pRtnL2DepthField->AskPrice7);
    assign(RtnL2DepthFieldJson["AskVolume7"], pRtnL2DepthField->AskVolume7);
    assign(RtnL2DepthFieldJson["BidPrice7"], pRtnL2DepthField->BidPrice7);
    assign(RtnL2DepthFieldJson["BidVolume7"], pRtnL2DepthField->BidVolume7);
    assign(RtnL2DepthFieldJson["AskPrice8"], pRtnL2DepthField->AskPrice8);
    assign(RtnL2DepthFieldJson["AskVolume8"], pRtnL2DepthField->AskVolume8);
    assign(RtnL2DepthFieldJson["BidPrice8"], pRtnL2DepthField->BidPrice8);
    assign(RtnL2DepthFieldJson["BidVolume8"], pRtnL2DepthField->BidVolume8);
    assign(RtnL2DepthFieldJson["AskPrice9"], pRtnL2DepthField->AskPrice9);
    assign(RtnL2DepthFieldJson["AskVolume9"], pRtnL2DepthField->AskVolume9);
    assign(RtnL2DepthFieldJson["BidPrice9"], pRtnL2DepthField->BidPrice9);
    assign(RtnL2DepthFieldJson["BidVolume9"], pRtnL2DepthField->BidVolume9);
    assign(RtnL2DepthFieldJson["AskPrice10"], pRtnL2DepthField->AskPrice10);
    assign(RtnL2DepthFieldJson["AskVolume10"], pRtnL2DepthField->AskVolume10);
    assign(RtnL2DepthFieldJson["BidPrice10"], pRtnL2DepthField->BidPrice10);
    assign(RtnL2DepthFieldJson["BidVolume10"], pRtnL2DepthField->BidVolume10);
}
void RedisPublisher::handleRtnL2TradeField(nlohmann::json& RtnL2TradeFieldJson, const CUTRtnL2TradeField* pRtnL2TradeField)
{
    assign(RtnL2TradeFieldJson["ExchangeTime"], pRtnL2TradeField->ExchangeTime);
    assign(RtnL2TradeFieldJson["LocalTime"], pRtnL2TradeField->LocalTime);
    assign(RtnL2TradeFieldJson["ArriveTime"], pRtnL2TradeField->ArriveTime);
    assign(RtnL2TradeFieldJson["PlatformTime"], pRtnL2TradeField->PlatformTime);
    assign(RtnL2TradeFieldJson["ExchangeID"], pRtnL2TradeField->ExchangeID);
    assign(RtnL2TradeFieldJson["InstrumentID"], pRtnL2TradeField->InstrumentID);
    assign(RtnL2TradeFieldJson["LastPrice"], pRtnL2TradeField->LastPrice);
    assign(RtnL2TradeFieldJson["Side"], getSideString(pRtnL2TradeField->Side));
    assign(RtnL2TradeFieldJson["CeilingPrice"], pRtnL2TradeField->CeilingPrice);
    assign(RtnL2TradeFieldJson["FloorPrice"], pRtnL2TradeField->FloorPrice);
    assign(RtnL2TradeFieldJson["BestAsk"], pRtnL2TradeField->BestAsk);
    assign(RtnL2TradeFieldJson["BestBid"], pRtnL2TradeField->BestBid);
    assign(RtnL2TradeFieldJson["Quantity"], pRtnL2TradeField->Quantity);
    assign(RtnL2TradeFieldJson["TotalQuantity"], pRtnL2TradeField->TotalQuantity);
}
void RedisPublisher::handleRtnL2OrderField(nlohmann::json& RtnL2OrderFieldJson, const CUTRtnL2OrderField* pRtnL2OrderField)
{
    assign(RtnL2OrderFieldJson["ExchangeTime"], pRtnL2OrderField->ExchangeTime);
    assign(RtnL2OrderFieldJson["LocalTime"], pRtnL2OrderField->LocalTime);
    assign(RtnL2OrderFieldJson["ArriveTime"], pRtnL2OrderField->ArriveTime);
    assign(RtnL2OrderFieldJson["PlatformTime"], pRtnL2OrderField->PlatformTime);
    assign(RtnL2OrderFieldJson["ExchangeID"], pRtnL2OrderField->ExchangeID);
    assign(RtnL2OrderFieldJson["InstrumentID"], pRtnL2OrderField->InstrumentID);
    assign(RtnL2OrderFieldJson["Price"], pRtnL2OrderField->Price);
    assign(RtnL2OrderFieldJson["Volume"], pRtnL2OrderField->Volume);
}
void RedisPublisher::handleRtnL2IndexField(nlohmann::json& RtnL2IndexFieldJson, const CUTRtnL2IndexField* pRtnL2IndexField)
{
    assign(RtnL2IndexFieldJson["ExchangeTime"], pRtnL2IndexField->ExchangeTime);
    assign(RtnL2IndexFieldJson["LocalTime"], pRtnL2IndexField->LocalTime);
    assign(RtnL2IndexFieldJson["ArriveTime"], pRtnL2IndexField->ArriveTime);
    assign(RtnL2IndexFieldJson["PlatformTime"], pRtnL2IndexField->PlatformTime);
    assign(RtnL2IndexFieldJson["ExchangeID"], pRtnL2IndexField->ExchangeID);
    assign(RtnL2IndexFieldJson["InstrumentID"], pRtnL2IndexField->InstrumentID);
    assign(RtnL2IndexFieldJson["PreCloseIndex"], pRtnL2IndexField->PreCloseIndex);
    assign(RtnL2IndexFieldJson["OpenIndex"], pRtnL2IndexField->OpenIndex);
    assign(RtnL2IndexFieldJson["CloseIndex"], pRtnL2IndexField->CloseIndex);
    assign(RtnL2IndexFieldJson["HighIndex"], pRtnL2IndexField->HighIndex);
    assign(RtnL2IndexFieldJson["LowIndex"], pRtnL2IndexField->LowIndex);
    assign(RtnL2IndexFieldJson["LastIndex"], pRtnL2IndexField->LastIndex);
    assign(RtnL2IndexFieldJson["TurnOver"], pRtnL2IndexField->TurnOver);
    assign(RtnL2IndexFieldJson["TotalVolume"], pRtnL2IndexField->TotalVolume);
}
void RedisPublisher::handleRtnBarMarketDataField(nlohmann::json& RtnBarMarketDataFieldJson, const CUTRtnBarMarketDataField* pRtnBarMarketDataField)
{
    assign(RtnBarMarketDataFieldJson["TradingDay"], pRtnBarMarketDataField->TradingDay);
    assign(RtnBarMarketDataFieldJson["InstrumentID"], pRtnBarMarketDataField->InstrumentID);
    assign(RtnBarMarketDataFieldJson["CeilingPrice"], pRtnBarMarketDataField->CeilingPrice);
    assign(RtnBarMarketDataFieldJson["FloorPrice"], pRtnBarMarketDataField->FloorPrice);
    assign(RtnBarMarketDataFieldJson["StartUpdateTime"], pRtnBarMarketDataField->StartUpdateTime);
    assign(RtnBarMarketDataFieldJson["EndUpdateTime"], pRtnBarMarketDataField->EndUpdateTime);
    assign(RtnBarMarketDataFieldJson["Open"], pRtnBarMarketDataField->Open);
    assign(RtnBarMarketDataFieldJson["Close"], pRtnBarMarketDataField->Close);
    assign(RtnBarMarketDataFieldJson["Low"], pRtnBarMarketDataField->Low);
    assign(RtnBarMarketDataFieldJson["High"], pRtnBarMarketDataField->High);
    assign(RtnBarMarketDataFieldJson["Volume"], pRtnBarMarketDataField->Volume);
    assign(RtnBarMarketDataFieldJson["StartVolume"], pRtnBarMarketDataField->StartVolume);
}

long long RedisPublisher::get_publish_id(unsigned int type)
{
    auto iter_find_type = publish_ids.find(type);
    if (iter_find_type!=publish_ids.end())
    {
        return ++iter_find_type->second;
    }
    else
    {
        publish_ids.emplace(type, 0);
        return 0; 
    }
}

void RedisPublisher::publish_buffer_content()
{
    for_each(position_buffer_.begin(), position_buffer_.end(), [&](PublishPositionType::value_type key_content)
    {
        nlohmann::json position;
        assign(position["SequenceNo"], get_publish_id(UT_TID_RtnPosition));
        assign(position["ExchangeID"], key_content.second->ExchangeID);
        assign(position["AccountName"], key_content.second->AccountName);
        assign(position["AccountType"], getAccountTypeString(key_content.second->AccountType));
        assign(position["InstrumentID"], key_content.second->InstrumentID);
        assign(position["PosiDirection"], key_content.second->PosiDirection);
        assign(position["Position"], key_content.second->Position);
        assign(position["YDPosition"], key_content.second->YDPosition);
        assign(position["Price"], key_content.second->Price);
        assign(position["Frozen"], key_content.second->Frozen);
        assign(position["Available"], key_content.second->Available);
        assign(position["TotalAvail"], key_content.second->TotalAvail);
        assign(position["UpdateTime"], key_content.second->UpdateTime);
        assign(position["CreateTime"], key_content.second->CreateTime);
        assign(position["CurrencyID"], key_content.second->CurrencyID);
        assign(position["BaseCurrency"], key_content.second->BaseCurrency);
        assign(position["OrderMargin"], key_content.second->OrderMargin);
        assign(position["PositionMargin"], key_content.second->PositionMargin);
        assign(position["FrozenBuy"], key_content.second->FrozenBuy);
        assign(position["FrozenSell"], key_content.second->FrozenSell);
        assign(position["PositionID"], key_content.second->PositionID);
        assign(position["AssetType"], key_content.second->AssetType);
        assign(position["TradeChannel"], key_content.second->TradeChannel);

        boost::shared_ptr<string> content{new string{position.dump()}};
        publish(UT_TID_RtnPosition, content);
    });
    position_buffer_.clear();
    for_each(account_buffer_.begin(), account_buffer_.end(), [&](PublishAccountType::value_type key_content)
    {
        nlohmann::json account;
        assign(account["SequenceNo"], get_publish_id(UT_TID_RtnAccount));
        assign(account["ExchangeID"], key_content.second->ExchangeID);
        assign(account["AccountName"], key_content.second->AccountName);
        assign(account["AccountType"], getAccountTypeString(key_content.second->AccountType));
        assign(account["CurrencyName"], key_content.second->CurrencyName);
        assign(account["CurrencyQuantity"], key_content.second->CurrencyQuantity);
        assign(account["PositionMargin"], key_content.second->PositionMargin);
        assign(account["OrderMargin"], key_content.second->OrderMargin);
        assign(account["PositionBalance"], key_content.second->PositionBalance);
        assign(account["TotalBalance"], key_content.second->TotalBalance);
        assign(account["Available"], key_content.second->Available);
        assign(account["LongAvailable"], key_content.second->LongAvailable);
        assign(account["ShortAvailable"], key_content.second->ShortAvailable);
        assign(account["ActualLongAvail"], key_content.second->ActualLongAvail);
        assign(account["ActualShortAvail"], key_content.second->ActualShortAvail);
        assign(account["Frozen"], key_content.second->Frozen);
        assign(account["Fee"], key_content.second->Fee);
        assign(account["FrozenBuy"], key_content.second->FrozenBuy);
        assign(account["FrozenSell"], key_content.second->FrozenSell);
        assign(account["UpdateTime"], key_content.second->UpdateTime);
        assign(account["CurrencyID"], key_content.second->CurrencyID);
        assign(account["AssetType"], key_content.second->AssetType);
        assign(account["TradeChannel"], key_content.second->TradeChannel);
        assign(account["Borrow"], key_content.second->Borrow);
        assign(account["Lend"], key_content.second->Lend);
        assign(account["DebtOffset"], key_content.second->DebtOffset);
        assign(account["TransferOffset"], key_content.second->TransferOffset);

        boost::shared_ptr<string> content{new string{account.dump()}};
        publish(UT_TID_RtnAccount, content);
    });
    account_buffer_.clear();


}
