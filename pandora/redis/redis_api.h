#pragma once
#include "../pandora_declare.h"
#include <mutex>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <atomic>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include "../util/thread_base.h"
#include "../util/thread_basepool.h"

struct redisAsyncContext;
struct event_base;
struct redisContext;

PANDORA_NAMESPACE_START

class UTLog;
// Redis call back class
class CRedisSpi
{
public:
    // redis connect notify
    virtual void OnConnected()= 0;
    // redis disconnect notify
    virtual void OnDisconnected(int status){}
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg){}
    // redis get message notify
    virtual void OnGetMessage(const std::string& key, const std::string& value){}
};

// publish content
struct PublishProperty
{
    PublishProperty(const string& sessionid, const string& channel, const string& msg):SessionID{sessionid}, Channel{channel}, Message{msg}{}
    string SessionID;    // identity for the channel
    string Channel;     // the publish channel
    string Message;     // the message for publish
};

// define the index
const int idx_identity_channel = 0;
const int idx_publish_sequence = 1;

// define the publish container
typedef boost::multi_index::multi_index_container<
	PublishProperty,
	boost::multi_index::indexed_by<
		boost::multi_index::hashed_unique<
            boost::multi_index::composite_key<
                PublishProperty,
                boost::multi_index::member<PublishProperty,std::string, &PublishProperty::SessionID>,
                boost::multi_index::member<PublishProperty,std::string, &PublishProperty::Channel>
            >
		>,
        boost::multi_index::sequenced<>
	>
> Publish_Container;

enum RedisMode
{
    RM_Subscribe=0,
    RM_Publish,
    RM_GetData,
    RM_PubSub
};

// Redis api, for synget, publish, subscibe from redis
class CRedisApi
{
public:
    using UTLogPtr = boost::shared_ptr<UTLog>;
public:
    CRedisApi(UTLogPtr logger);
    virtual ~CRedisApi();
    // register callback object
    void RegisterSpi(CRedisSpi* pSpi);
    // register server information
    // mode  0: subscribe 1: publish 2: syn get data 3: publish & subscribe
    int RegisterRedis(const std::string& host, int port, const std::string& auth="", RedisMode mode = RM_Subscribe);
    // subscribe topic
    void SubscribeTopic(const std::string& topic);
    // unsubscribe topic
    void UnSubscribeTopic(const std::string& topic);
    // psubscribe topic
    void PSubscribeTopic(const std::string& topic);
    // punsubscribe topic
    void PUnSubscribeTopic(const std::string& topic);
    // retrieve spi
    CRedisSpi* GetSpi(){return RedisSpi;}
    // retrieve logger
    UTLogPtr GetLogger(){return Logger;}
    // publish topic
    // void Publish(const string& channel, const string& content, const string& key, bool aggregate=false);

//     // set value
//     void AsyncSet(const string& key, const string& value);
//     // get value
//     void AsyncGet(const string& key);
    // // select database
    // void Select(int database);

    // keys
    list<string> Keys(const string &pattern);
    // get & set
    std::string SyncGet(const string& key);
    void SyncSet(const string& key, const string& value);
    // hget & hset & hdel
    string HGet(const string& key, const string& field);
    list<pair<string, string> > HGetAll(const string& key);
    void HSet(const string& key, const string& field, const string& value);
    void HDel(const string& key, const string& field);
    // sadd & srem & spop(随机返回并删除名称为 key 的 set 中一个元素)
    void SAdd(const string &set, const string& member);
    void SRem(const string &set, const string& member);
//    string SPop(const string &set);
    set<string> SMembers(const string &set);


    // sync select database
    void SyncSelect(int database);


protected:
    // sync redis context
    redisContext* SyncRedisContext;
private:
    // start running
    void Start();
    int InitAsyncRedisContext();
    int InitSyncRedisContext();
    // 
    static void OnMessage_Callback(redisAsyncContext *c, void *r, void *privdata);
    static void OnGetMessage_Callback(redisAsyncContext *c, void *r, void *privdata);
    static void OnConnect_Callback(const redisAsyncContext *c, int status);
    static void OnDisconnect_Callback(const redisAsyncContext *c, int status);

    std::string Host;               // host server address
    int Port;                       // port number  
    std::string Auth;               // auth
    RedisMode RedisAccessMode;      // the mode how use redis

    // libevent base
    event_base* EventBase;
    // redis spi
    CRedisSpi*   RedisSpi{nullptr};
    // redis context for subscribe
    redisAsyncContext* AsyncRedisContext;
    // UTlogger
    UTLogPtr Logger;

    // thread receive data
    ThreadPtr AsyncThread;
    // stop async
    std::atomic<bool> StopAsync{false};

    //publish thread
    boost::shared_ptr<std::thread> PublishThread;
    // condition variable
	std::condition_variable ContainerCodition;
    // stop thread
	std::atomic<bool> StopPublish{false};
    // publish container
    Publish_Container PublishContent;
    // publish sequenceno
    std::map<string, long long> PublishSequenceNo;
    // handle publish count
    const int HandleCountOnce{1};
    // mutex publish
    // std::mutex MutexPublish;
    // synContext connect
    bool syncontext_connected_{false};
};
DECLARE_PTR(CRedisApi);

class CRedisApiPublish : public CRedisApi, public ThreadBasePool
{
public:
    CRedisApiPublish(CRedisApi::UTLogPtr logger, io_service_pool& pool);
    virtual ~CRedisApiPublish();

    void Publish(boost::shared_ptr<string> channel, boost::shared_ptr<string> content);

    // handle publish event
    void HandlePublish(boost::shared_ptr<string> channel, boost::shared_ptr<string> content);
};
DECLARE_PTR(CRedisApiPublish);

PANDORA_NAMESPACE_END