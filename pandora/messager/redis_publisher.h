/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UTType.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file redis_publisher.h
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190723      创建该文件
/////////////////////////////////////////////////////////////////////////


//判断字符串中是否有rtn字段

//判断字符串中是否有特定字符串



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
	
    void handle_rsp_info(PackagePtr package);
    void handle_req_create_order(PackagePtr package);
    void handle_rsp_create_order(PackagePtr package);
    void handle_rtn_order(PackagePtr package);
    void handle_rtn_trade(PackagePtr package);
    void handle_req_cancel_order(PackagePtr package);
    void handle_rsp_cancel_order(PackagePtr package);
    void handle_sub_position(PackagePtr package);
    void handle_rtn_position(PackagePtr package);
    void handle_req_qry_order(PackagePtr package);
    void handle_rsp_qry_order(PackagePtr package);
    void handle_req_qry_trade(PackagePtr package);
    void handle_rsp_qry_trade(PackagePtr package);
    void handle_req_qry_account(PackagePtr package);
    void handle_rsp_qry_account(PackagePtr package);
    void handle_sub_account(PackagePtr package);
    void handle_rtn_account(PackagePtr package);
    void handle_req_login(PackagePtr package);
    void handle_rsp_login(PackagePtr package);
    void handle_req_logout(PackagePtr package);
    void handle_rsp_logout(PackagePtr package);
    void handle_req_qry_position(PackagePtr package);
    void handle_rsp_qry_position(PackagePtr package);
    void handle_rtn_platform_detail(PackagePtr package);
    void handle_rtn_strategy_detail(PackagePtr package);
    void handle_rtn_depth(PackagePtr package);
    void handle_rtn_l2_depth(PackagePtr package);
    void handle_rtn_l2_trade(PackagePtr package);
    void handle_rtn_l2_order(PackagePtr package);
    void handle_rtn_l2_index(PackagePtr package);
    void handle_rtn_bar_market_data(PackagePtr package);
    void handle_rtn_business_debt(PackagePtr package);
    void handle_req_qry_account_business(PackagePtr package);
    void handle_rsp_qry_account_business(PackagePtr package);
    void handle_rtn_account_business(PackagePtr package);
    void handle_req_manual_transact(PackagePtr package);
    void handle_req_transact(PackagePtr package);
    void handle_rsp_transact(PackagePtr package);
    void handle_rtn_transact(PackagePtr package);
    void handle_req_qry_transact(PackagePtr package);
    void handle_rsp_qry_transact(PackagePtr package);

    /// 响应信息
    void handleRspInfoField(nlohmann::json& RspInfoFieldJson, const CUTRspInfoField* pRspInfoField);    
    /// 请求下单
    void handleReqCreateOrderField(nlohmann::json& ReqCreateOrderFieldJson, const CUTReqCreateOrderField* pReqCreateOrderField);    
    /// 请求下单返回
    void handleRspCreateOrderField(nlohmann::json& RspCreateOrderFieldJson, const CUTRspCreateOrderField* pRspCreateOrderField, const CUTRspInfoField* pRspInfoField);
    /// 报单通知
    void handleRtnOrderField(nlohmann::json& RtnOrderFieldJson, const CUTRtnOrderField* pRtnOrderField);    
    /// 成交通知
    void handleRtnTradeField(nlohmann::json& RtnTradeFieldJson, const CUTRtnTradeField* pRtnTradeField);    
    /// 请求撤单
    void handleReqCancelOrderField(nlohmann::json& ReqCancelOrderFieldJson, const CUTReqCancelOrderField* pReqCancelOrderField);    
    /// 请求撤单返回
    void handleRspCancelOrderField(nlohmann::json& RspCancelOrderFieldJson, const CUTRspCancelOrderField* pRspCancelOrderField, const CUTRspInfoField* pRspInfoField);
    /// 请求订阅持仓信息
    void handleSubPositionField(nlohmann::json& SubPositionFieldJson, const CUTSubPositionField* pSubPositionField);    
    /// 持仓信息通知
    void handleRtnPositionField(nlohmann::json& RtnPositionFieldJson, const CUTRtnPositionField* pRtnPositionField);    
    /// 查询报单
    void handleReqQryOrderField(nlohmann::json& ReqQryOrderFieldJson, const CUTReqQryOrderField* pReqQryOrderField);    
    /// 返回查询报单
    void handleRspQryOrderField(nlohmann::json& RspQryOrderFieldJson, const CUTRspQryOrderField* pRspQryOrderField, const CUTRspInfoField* pRspInfoField);
    /// 查询成交
    void handleReqQryTradeField(nlohmann::json& ReqQryTradeFieldJson, const CUTReqQryTradeField* pReqQryTradeField);    
    /// 返回查询成交
    void handleRspQryTradeField(nlohmann::json& RspQryTradeFieldJson, const CUTRspQryTradeField* pRspQryTradeField, const CUTRspInfoField* pRspInfoField);
    /// 查询账户
    void handleReqQryAccountField(nlohmann::json& ReqQryAccountFieldJson, const CUTReqQryAccountField* pReqQryAccountField);    
    /// 返回查询账户
    void handleRspQryAccountField(nlohmann::json& RspQryAccountFieldJson, const CUTRspQryAccountField* pRspQryAccountField, const CUTRspInfoField* pRspInfoField);
    /// 请求订阅账户信息
    void handleSubAccountField(nlohmann::json& SubAccountFieldJson, const CUTSubAccountField* pSubAccountField);    
    /// 账户信息通知
    void handleRtnAccountField(nlohmann::json& RtnAccountFieldJson, const CUTRtnAccountField* pRtnAccountField);    
    /// 请求登录
    void handleReqLoginField(nlohmann::json& ReqLoginFieldJson, const CUTReqLoginField* pReqLoginField);    
    /// 返回登录
    void handleRspLoginField(nlohmann::json& RspLoginFieldJson, const CUTRspLoginField* pRspLoginField, const CUTRspInfoField* pRspInfoField);
    /// 请求登出
    void handleReqLogoutField(nlohmann::json& ReqLogoutFieldJson, const CUTReqLogoutField* pReqLogoutField);    
    /// 返回登出
    void handleRspLogoutField(nlohmann::json& RspLogoutFieldJson, const CUTRspLogoutField* pRspLogoutField, const CUTRspInfoField* pRspInfoField);
    /// 请求登出
    void handleReqQryPositionField(nlohmann::json& ReqQryPositionFieldJson, const CUTReqQryPositionField* pReqQryPositionField);    
    /// 返回登出
    void handleRspQryPositionField(nlohmann::json& RspQryPositionFieldJson, const CUTRspQryPositionField* pRspQryPositionField, const CUTRspInfoField* pRspInfoField);
    /// 平台状态
    void handleRtnPlatformDetailField(nlohmann::json& RtnPlatformDetailFieldJson, const CUTRtnPlatformDetailField* pRtnPlatformDetailField);    
    /// 策略详情通知
    void handleRtnStrategyDetailField(nlohmann::json& RtnStrategyDetailFieldJson, const CUTRtnStrategyDetailField* pRtnStrategyDetailField);    
    /// Apollo子业务借贷关系
    void handleRtnBusinessDebtField(nlohmann::json& RtnBusinessDebtFieldJson, const CUTRtnBusinessDebtField* pRtnBusinessDebtField);    
    /// 查询账户
    void handleReqQryAccountBusinessField(nlohmann::json& ReqQryAccountBusinessFieldJson, const CUTReqQryAccountBusinessField* pReqQryAccountBusinessField);    
    /// 返回查询账户
    void handleRspQryAccountBusinessField(nlohmann::json& RspQryAccountBusinessFieldJson, const CUTRspQryAccountBusinessField* pRspQryAccountBusinessField, const CUTRspInfoField* pRspInfoField);
    /// 请求订阅账户信息
    void handleRtnAccountBusinessField(nlohmann::json& RtnAccountBusinessFieldJson, const CUTRtnAccountBusinessField* pRtnAccountBusinessField);    
    /// 交易所充值
    void handleReqManualTransactField(nlohmann::json& ReqManualTransactFieldJson, const CUTReqManualTransactField* pReqManualTransactField);    
    /// 请求转账记录
    void handleReqTransactField(nlohmann::json& ReqTransactFieldJson, const CUTReqTransactField* pReqTransactField);    
    /// 返回转账记录
    void handleRspTransactField(nlohmann::json& RspTransactFieldJson, const CUTRspTransactField* pRspTransactField, const CUTRspInfoField* pRspInfoField);
    /// 转账记录通知
    void handleRtnTransactField(nlohmann::json& RtnTransactFieldJson, const CUTRtnTransactField* pRtnTransactField);    
    /// 请求查询转账记录
    void handleReqQryTransactField(nlohmann::json& ReqQryTransactFieldJson, const CUTReqQryTransactField* pReqQryTransactField);    
    /// 请求转账记录响应
    void handleRspQryTransactField(nlohmann::json& RspQryTransactFieldJson, const CUTRspQryTransactField* pRspQryTransactField, const CUTRspInfoField* pRspInfoField);
    /// 深度行情通知
    void handleRtnDepthField(nlohmann::json& RtnDepthFieldJson, const CUTRtnDepthField* pRtnDepthField);    
    /// L2深度行情通知
    void handleRtnL2DepthField(nlohmann::json& RtnL2DepthFieldJson, const CUTRtnL2DepthField* pRtnL2DepthField);    
    /// L2成交通知
    void handleRtnL2TradeField(nlohmann::json& RtnL2TradeFieldJson, const CUTRtnL2TradeField* pRtnL2TradeField);    
    /// L2报单通知
    void handleRtnL2OrderField(nlohmann::json& RtnL2OrderFieldJson, const CUTRtnL2OrderField* pRtnL2OrderField);    
    /// L2指数通知
    void handleRtnL2IndexField(nlohmann::json& RtnL2IndexFieldJson, const CUTRtnL2IndexField* pRtnL2IndexField);    
    /// K线通知
    void handleRtnBarMarketDataField(nlohmann::json& RtnBarMarketDataFieldJson, const CUTRtnBarMarketDataField* pRtnBarMarketDataField);    
    
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
