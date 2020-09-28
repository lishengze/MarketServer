#pragma once
#include "../pandora_declare.h"

PANDORA_NAMESPACE_START

#define LOG_TRACE(content) \
    if (logger_) UT_LOG_TRACE(logger_, content)

#define LOG_DEBUG(content) \
    if (logger_) UT_LOG_DEBUG(logger_, content)

#define LOG_INFO(content) \
    if (logger_) UT_LOG_INFO(logger_, content)

#define LOG_WARN(content) \
    if (logger_) UT_LOG_WARNING(logger_, content)

#define LOG_ERROR(content) \
    if (logger_) UT_LOG_ERROR(logger_, content)

#define DINGTALK(title, level, errorid, errormsg) \
    if(talker_) talker_->send_message(title, level, errorid, errormsg)

#define PUBLISH(topic, message, sessionid, aggregate) \
    if (redis_publisher_) redis_publisher_->publish(topic, message, sessionid, aggregate);

#define PUBLISH_ACCOUNT(account)   \
    if (redis_publisher_) redis_publisher_->publish_account(account)

#define PUBLISH_POSITION(position)   \
    if (redis_publisher_) redis_publisher_->publish_position(position)

class RedisPublisher;
class IRedisPublisher
{
public:
    void set_publisher(boost::shared_ptr<RedisPublisher> publisher){redis_publisher_=publisher;}
protected:
    boost::shared_ptr<RedisPublisher> redis_publisher_;
};

class UTLog;
class DingTalk;
class IMessager
{
public:
    void set_logger(boost::shared_ptr<UTLog> logger){logger_=logger;}
    void set_talker(boost::shared_ptr<DingTalk> talk){talker_=talk;}
protected:
    boost::shared_ptr<UTLog> logger_;
    boost::shared_ptr<DingTalk> talker_;
};

PANDORA_NAMESPACE_END