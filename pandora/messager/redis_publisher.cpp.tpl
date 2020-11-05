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

!!include ../../quark/common.tpl.h!!


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

!!enter UT!!
!!enter packages!!
!!let size=@son_size!!
!!leave!!
boost::shared_ptr<string> RedisPublisher::publish_type_str(unsigned int type)
{
    switch (type)
    {
        !!travel packages!!
        case !!@head!!_TID_!!@name!!:
            return boost::shared_ptr<string>{new string{service_name_.empty() ? "!!@name!!" : service_name_+"."+"!!@name!!"}};
        !!if @pumpid +1 == atoi(@size)!!
        default:
            UT_LOG_ERROR_FMT(logger_, "Publish Type Error: %d ", type);
            return nullptr;
        !!endif!!
        !!next!!
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
        !!travel packages!!
            !!let JOINT=joint_capital(@name)!!
        case UT_TID_!!@name!!:
            handle!!@JOINT!!(package);
            break;    
            !!if @pumpid +1 == atoi(@size)!! 
        default:
            UT_LOG_ERROR_FMT(logger_, "Publish Type Error: %d ", package->Tid());
            !!endif!!
        !!next!!
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

    !!travel fields!!
    !!let filed_name=@name!!
    !!if !strcmp(@filed_name,"RtnBusinessDebt")!!
    nlohmann::json !!@filed_name!!FieldJson;
    !!travel self!!
        !!if !strcmp(@print_type,"t")!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], get!!@name!!String(p!!@filed_name!!Field->!!@name!!));
        !!else!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], p!!@filed_name!!Field->!!@name!!);
        !!endif!!
    !!next!!
    boost::shared_ptr<string> content{new string{!!@filed_name!!FieldJson.dump()}};
    !!endif!!
    !!next!!


    publish(UT_TID_RtnBusinessDebt, content);
}

void RedisPublisher::handle_publish_account_nodelay(boost::shared_ptr<CUTRtnAccountField> pRtnAccountField)
{

    !!travel fields!!
    !!let filed_name=@name!!
    !!if !strcmp(@filed_name,"RtnAccount")!!
    nlohmann::json !!@filed_name!!FieldJson;
    !!travel self!!
        !!if !strcmp(@print_type,"t")!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], get!!@name!!String(p!!@filed_name!!Field->!!@name!!));
        !!else!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], p!!@filed_name!!Field->!!@name!!);
        !!endif!!
    !!next!!
    boost::shared_ptr<string> content{new string{!!@filed_name!!FieldJson.dump()}};
    !!endif!!
    !!next!!

    publish(UT_TID_RtnAccount, content);
}

!!travel packages!!
!!let package_name=@name!!
!!let JOINT=joint_capital(@name)!!
void RedisPublisher::handle!!@JOINT!!(PackagePtr package)
{
    nlohmann::json !!@package_name!!FieldJson;
    long long sequence_no{get_publish_id(UT_TID_!!@package_name!!)};

    assign(!!@package_name!!FieldJson["SequenceNo"], sequence_no);
!!if !strncmp(@package_name, "Rsp", 3) && strncmp(@package_name, "RspInfo", 7)!!
    const CUT!!@name!!Field* p!!@name!!Field = GET_FIELD(package, CUT!!@name!!Field);
    const CUTRspInfoField* pRspInfoField = GET_FIELD(package, CUTRspInfoField);
    long requestID{package->RequestID()};
    assign(!!@package_name!!FieldJson["RequestID"], requestID);
    handle!!@package_name!!Field(!!@package_name!!FieldJson, p!!@name!!Field, pRspInfoField);
!!elseif !strncmp(@package_name, "Req", 3)!!
    const CUT!!@name!!Field* p!!@name!!Field = GET_FIELD(package, CUT!!@name!!Field);
    long requestID{package->RequestID()};
    assign(!!@package_name!!FieldJson["RequestID"], requestID);
    handle!!@package_name!!Field(!!@package_name!!FieldJson, p!!@name!!Field);
!!else!!
    const CUT!!@name!!Field* p!!@name!!Field = GET_FIELD(package, CUT!!@name!!Field);
    handle!!@package_name!!Field(!!@package_name!!FieldJson, p!!@name!!Field);
!!endif!!

    //std::cout<<!!@package_name!!FieldJson.dump()<<std::endl;
    publish(UT_TID_!!@package_name!!, !!@package_name!!FieldJson.dump());
}

!!next!!

!!travel fields!!
!!let filed_name=@name!!
!!if strncmp(@filed_name,"Rsp",3) || !strncmp(@filed_name, "RspInfo", 7)!!
void RedisPublisher::handle!!@filed_name!!Field(nlohmann::json& !!@filed_name!!FieldJson, const CUT!!@filed_name!!Field* p!!@filed_name!!Field)
{
    !!travel self!!
        !!if !strcmp(@print_type,"t")!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], get!!@type!!String(p!!@filed_name!!Field->!!@name!!));
        !!else!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], p!!@filed_name!!Field->!!@name!!);
        !!endif!!
    !!next!!
}
!!else!!
void RedisPublisher::handle!!@filed_name!!Field(nlohmann::json& !!@filed_name!!FieldJson, const CUT!!@filed_name!!Field* p!!@filed_name!!Field, const CUTRspInfoField* pRspInfoField)
{
    !!travel self!!
        !!if !strcmp(@print_type,"t")!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], get!!@type!!String(p!!@filed_name!!Field->!!@name!!));
        !!else!!
    assign(!!@filed_name!!FieldJson["!!@name!!"], p!!@filed_name!!Field->!!@name!!);
        !!endif!!
    !!next!!
    assign(!!@filed_name!!FieldJson["ErrorID"], pRspInfoField->ErrorID);
    assign(!!@filed_name!!FieldJson["ErrorMsg"], pRspInfoField->ErrorMsg);
}
!!endif!!
!!next!!

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
    !!travel fields!!
        !!if !strcmp(@name,"RtnAccount")!!
    for_each(account_buffer_.begin(), account_buffer_.end(), [&](PublishAccountType::value_type key_content)
    {
        nlohmann::json account;
        assign(account["SequenceNo"], get_publish_id(UT_TID_!!@name!!));
                !!travel self!!
                    !!if !strcmp(@name,"AccountType")!!
        assign(account["!!@name!!"], getAccountTypeString(key_content.second->!!@name!!));
                    !!else!!
        assign(account["!!@name!!"], key_content.second->!!@name!!);
                    !!endif!!
                !!next!!

        boost::shared_ptr<string> content{new string{account.dump()}};
        publish(UT_TID_RtnAccount, content);
    });
    account_buffer_.clear();

        !!elseif !strcmp(@name,"RtnPosition")!!
    for_each(position_buffer_.begin(), position_buffer_.end(), [&](PublishPositionType::value_type key_content)
    {
        nlohmann::json position;
        assign(position["SequenceNo"], get_publish_id(UT_TID_!!@name!!));
                !!travel self!!
                    !!if !strcmp(@name,"AccountType")!!
        assign(position["!!@name!!"], getAccountTypeString(key_content.second->!!@name!!));
                    !!else!!
        assign(position["!!@name!!"], key_content.second->!!@name!!);
                    !!endif!!
                !!next!!

        boost::shared_ptr<string> content{new string{position.dump()}};
        publish(UT_TID_RtnPosition, content);
    });
    position_buffer_.clear();
        !!endif!!
    !!next!!

}
!!leave!!