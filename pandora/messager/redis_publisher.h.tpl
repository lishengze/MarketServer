/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UTType.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file redis_publisher.h
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190723      创建该文件
/////////////////////////////////////////////////////////////////////////

!!include ../../quark/common.tpl.h!!

#pragma once
#include "../pandora_declare.h"
#include "messager.h"
#include "../util/singleton.hpp"
#include "ut_log.h"
#include <atomic>
#include "../util/thread_basepool.h"
#include "quark/cxx/ut/UtData.h"
#include "../package/package.h"
#include "../util/json.hpp"
#include "deque"

// generate the singleton object
#define PUBLISHER utrade::pandora::Singleton<utrade::pandora::RedisPublisher>::Instance()

PANDORA_NAMESPACE_START

struct PublishContent
{
    PublishContent(unsigned int channel, boost::shared_ptr<string> content) : Channel{channel}, Content{content} {}
    unsigned int Channel;
    boost::shared_ptr<string> Content;
};

class CRedisApiPublish;
// redis publisher object
class RedisPublisher : public ThreadBasePool
{
    // using PublishContentType = std::unordered_map<std::string, boost::shared_ptr<std::string> >;
    using PublishAccountType = std::unordered_map<long, boost::shared_ptr<CUTRtnAccountField> >;
    using PublishPositionType = std::unordered_map<long, boost::shared_ptr<CUTRtnPositionField> >;
    using PublishBusinessDebt = std::deque<boost::shared_ptr<CUTRtnBusinessDebtField>>;

public:
    RedisPublisher(io_service_pool& pool, const std::string& srv_name="");
    // RedisPublisher(const string& host, int port, const string& auth="");
    virtual ~RedisPublisher();

    // set redis server config
    void set_server_config(const string& host, int port, const string& auth, UTLogPtr logger);
    // 发布各种数据包类型
    void publish_package(PackagePtr package);
    // 发布普通的信息
    void publish_message(const string& channel_name, const string& message);
    // 发布账户信息
    void publish_account(boost::shared_ptr<CUTRtnAccountField>);
    // 发布持仓信息
    void publish_position(boost::shared_ptr<CUTRtnPositionField>);
    //用于Apollo 发布子业务的借贷信息，附带账户信息
    void publish_business_debt(boost::shared_ptr<CUTRtnBusinessDebtField>);
    //用于Apollo 推送账户时不延时和不聚合推送
    void publish_account_nodelay(boost::shared_ptr<CUTRtnAccountField>);

private:
    // publish some message here
    void publish(unsigned int channel, boost::shared_ptr<string> content);
    void publish(unsigned int channel_ident, const string& content_str);
    // get publish type string
    boost::shared_ptr<string> publish_type_str(unsigned int type);
    // on timer event
    void on_timer(int millisecond);
    // queue the aggregate content 
    void queue_publish_content(unsigned int type, const std::string& key, boost::shared_ptr<string> content);
    // publish buffer content
    void publish_buffer_content();
    // // get publish identity
    long long get_publish_id(unsigned int type);
    void handle_publish_message(boost::shared_ptr<string> channel_name, boost::shared_ptr<string> content);
    void handle_publish_package(PackagePtr package);
    void handle_publish_account(boost::shared_ptr<CUTRtnAccountField>);
    void handle_publish_position(boost::shared_ptr<CUTRtnPositionField>);
	void handle_publish_business_debt(boost::shared_ptr<CUTRtnBusinessDebtField>);
    void handle_publish_account_nodelay(boost::shared_ptr<CUTRtnAccountField>);
	
!!enter UT!!
    !!travel packages!!
        !!let JOINT=joint_capital(@name)!!
    void handle!!@JOINT!!(PackagePtr package);
    !!next!!

    !!travel fields!!
    /// !!@comment!!
    !!let package_name=@name!!
!!if !strncmp(@package_name, "Rsp", 3) && strncmp(@package_name, "RspInfo", 7)!!
    void handle!!@package_name!!Field(nlohmann::json& !!@package_name!!FieldJson, const CUT!!@package_name!!Field* p!!@package_name!!Field, const CUTRspInfoField* pRspInfoField);
!!else!!
    void handle!!@package_name!!Field(nlohmann::json& !!@package_name!!FieldJson, const CUT!!@package_name!!Field* p!!@package_name!!Field);    
!!endif!!
!!next!!
!!leave!!
    
    void InitRedis();
    // define the redis ptr
    using RedisApiPublishPtr = boost::shared_ptr<utrade::pandora::CRedisApiPublish>;
    // service name
    std::string service_name_{""};
    // redis api, to send the request to server
    RedisApiPublishPtr redis_api_;
    // logger
    UTLogPtr logger_;

    // publisher id
    std::unordered_map<unsigned int, long long> publish_ids;
    // timer for call
    boost::shared_ptr<boost::asio::deadline_timer> timer_;
    // publish object
    PublishAccountType account_buffer_;
    PublishPositionType position_buffer_;
    PublishBusinessDebt business_debt_buffer_;
};

DECLARE_PTR(RedisPublisher);

PANDORA_NAMESPACE_END