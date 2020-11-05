#include "redis_api.h"
#include "hiredis/async.h"
#include "hiredis/hiredis.h"
#include "hiredis/adapters/libevent.h"
#include "../messager/ut_log.h"

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>


using namespace std;
USING_PANDORA_NAMESPACE;

// Redis api class
CRedisApi::CRedisApi(UTLogPtr logger)
{
    Host = "";
    Port = 0;
    Auth = "";
    RedisAccessMode = RM_Subscribe;

    // event base init
    this->EventBase = event_base_new();

    // redis context for subscribe
    this->AsyncRedisContext = nullptr;
    // sync context
    this->SyncRedisContext = nullptr;
    // 初始化Spi的值
    this->RedisSpi = nullptr;
    // 初始化日志存储器
    this->Logger = logger;
}

CRedisApi::~CRedisApi()
{
    if (PublishThread)
    {
        StopPublish.store(true);
        if (PublishThread->joinable())
            PublishThread->join();
    }
    if (AsyncThread)
    {
        StopAsync.store(true);
        if (AsyncThread->joinable())
            AsyncThread->join();
    }
}

// 注册回调对象
void CRedisApi::RegisterSpi(CRedisSpi* pSpi)
{
    this->RedisSpi = pSpi;
}

// 注册数据库连接信息
// mode  0: subscribe 1: publish 2: syn get data 3: publish & subscribe
int CRedisApi::RegisterRedis(const std::string& host, int port, const std::string& auth, RedisMode mode)
{

    Host = host;
    Port = port;
    Auth = auth;
    RedisAccessMode = mode;

    switch (RedisAccessMode)
    {
        case RM_Subscribe:
            InitAsyncRedisContext();
            break;
        case RM_Publish:
            InitSyncRedisContext();
            break;
        case RM_GetData:
            InitSyncRedisContext();
            break;
        case RM_PubSub:
            InitSyncRedisContext();
            InitAsyncRedisContext();
            break;
        default:
            return -1;
    }

    return 0;
}

int CRedisApi::InitAsyncRedisContext()
{
    try
    {
        if (AsyncRedisContext) return 0;
        //AsyncRedisContext
        // 请求连接数据库
        AsyncRedisContext = redisAsyncConnect(Host.c_str(), Port);
        if (AsyncRedisContext->err)
        {
            UT_LOG_ERROR_FMT(Logger, "[Redis] redis context error: %s Host: %s Port: %d", AsyncRedisContext->errstr, Host.c_str(), Port);
            AsyncRedisContext = nullptr;
            return 1;
        }
        // 数据库密码认证
        if (!Auth.empty())
        {
            std::string authCmd = std::string("auth ") + Auth;
            int ret = redisAsyncCommand(AsyncRedisContext, nullptr, nullptr, authCmd.c_str());
            if (ret != REDIS_OK) 
            {
                UT_LOG_ERROR(Logger, "[Redis] AUTH failed!\n");
                AsyncRedisContext = nullptr;
                return 1;
            }
        }

        // 设定自定义数据区
        AsyncRedisContext->data = this;
        // attach event base
        redisLibeventAttach(AsyncRedisContext, EventBase);
        // register connect and disconnect callback
        redisAsyncSetConnectCallback(AsyncRedisContext, OnConnect_Callback);
        redisAsyncSetDisconnectCallback(AsyncRedisContext, OnDisconnect_Callback);
        // init redis receive thread
        AsyncThread = ThreadPtr{new thread{&CRedisApi::Start, this}};
        return 0;
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::InitAsyncRedisContext exception: " << e.what() << "\n";
        return -1;
	}
    catch (...)
    {
        std::cerr << "CRedisApi::InitAsyncRedisContext Unknown Exception." << " \n";
        return -1;
    }
}

int CRedisApi::InitSyncRedisContext()
{
    try
    {
        if (SyncRedisContext) return 0;
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        SyncRedisContext = redisConnectWithTimeout(Host.c_str(), Port, timeout);
        if (SyncRedisContext == NULL || SyncRedisContext->err) {
            if (SyncRedisContext) {
                UT_LOG_ERROR_FMT(Logger, "[Redis] redis context error: %s Host: %s, Port: %d", SyncRedisContext->errstr, Host.c_str(), Port);
                redisFree(SyncRedisContext);
                SyncRedisContext = nullptr;
                return 1;
            } else {
                UT_LOG_ERROR(Logger, "Connection error: can't allocate redis context\n");
            }
        }

        // 数据库密码认证
        if (!Auth.empty())
        {
            std::string authCmd = std::string("auth ") + Auth;
            redisReply *reply = (redisReply*)redisCommand(SyncRedisContext, authCmd.c_str());
            if (!reply || reply->type == REDIS_REPLY_ERROR) 
            {
                UT_LOG_ERROR(Logger, "[Redis] AUTH failed!\n");
                return 1;
            }
        }
        // PublishThread = boost::shared_ptr<std::thread>(new thread(&CRedisApi::PublishRun, this));
        syncontext_connected_ = true;
        return 0;
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::InitSyncRedisContext exception: " << e.what() << "\n";
        return -1;
	}
    catch (...)
    {
        std::cerr << "CRedisApi::InitSyncRedisContext Unknown Exception." << " \n";
        return -1;
    }
}


void CRedisApi::Start()
{
    event_base_dispatch(EventBase);
}

void CRedisApi::OnMessage_Callback(redisAsyncContext *c, void *reply, void *privdata) 
{
    try
    {
        CRedisApi* pApi = static_cast<CRedisApi*>(c->data);
        if (!pApi)
        {
            std::cout << "[Redis] Failed Get Api Data!!!" << std::endl;
            return;
        }
        redisReply* r = (redisReply *)reply;
        if (reply == nullptr) 
        {
            UT_LOG_ERROR_FMT(pApi->GetLogger(), "[Redis] replay is null,error: %s", c->errstr);
            return;
        }

        if (privdata == nullptr) {
            UT_LOG_ERROR(pApi->GetLogger(), "[Redis] privdata is null");
            return;
        }

        if (r->type == REDIS_REPLY_ARRAY) 
        {
            std::string channel;
            std::string message;
            if (r->elements == 4)
            {
                if (r->element[2]->str != nullptr)
                {
                    channel = r->element[2]->str;
                }

                if (r->element[3]->str != nullptr)
                {
                    message = r->element[3]->str;
                }
            } 
            else if (r->elements == 3)
            {
                if (r->element[1]->str != nullptr) 
                {
                    channel = r->element[1]->str;
                }

                if (r->element[2]->str != nullptr) 
                {
                    message = r->element[2]->str;
                }
            } else {
                for (size_t j = 0; j < r->elements; j++)
                {
                    UT_LOG_INFO(pApi->GetLogger(), "[Redis] REDIS_REPLY_ARRAY: " << j << " : " << r->element[j]->str);
                }
                return;
            }

            if (channel.empty())
            {
                UT_LOG_ERROR(pApi->GetLogger(), "[Redis] channel is empty!");
                return;
            }
            if (message.empty())
            {
                // UT_LOG_ERROR_FMT(pApi->GetLogger(), "[Redis] channel: %s, message is empty!", channel.c_str());
                //SubscribeTopic success
                return;
            }
            // cout << "channel: " << channel << ", message: " << message << endl;
            if (pApi->GetSpi())
                pApi->GetSpi()->OnMessage(channel, message);
        }
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::OnMessage_Callback exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::OnMessage_Callback Unknown Exception." << " \n";
    }
}

void CRedisApi::OnGetMessage_Callback(redisAsyncContext *c, void *reply, void *privdata) 
{
    try
    {
        CRedisApi* pApi = static_cast<CRedisApi*>(c->data);
        if (!pApi)
        {
            std::cout << "[Redis] Failed Get Api Data!!!" << std::endl;
            return;
        }
        redisReply* r = (redisReply *)reply;
        if (reply == nullptr) 
        {
            UT_LOG_ERROR_FMT(pApi->GetLogger(), "[Redis] replay is null,error: %s", c->errstr);
            return;
        }

        if (privdata == nullptr) {
            UT_LOG_ERROR(pApi->GetLogger(), "[Redis] privdata is null");
            return;
        }

        //async get     delete
        if (r->type == REDIS_REPLY_STRING)
        {
            std::string key = *((std::string *)privdata);
            std::string value;
            if (r->str != nullptr)
            {
                value.append(r->str);
            }
            if (pApi->GetSpi())
                pApi->GetSpi()->OnMessage(key, value);
        }
        delete (std::string *)privdata;
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::OnGetMessage_Callback exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::OnGetMessage_Callback Unknown Exception." << " \n";
    }
}

void CRedisApi::OnConnect_Callback(const redisAsyncContext *c, int status) 
{
    try
    {
        CRedisApi* pApi = static_cast<CRedisApi*>(c->data);
        if (!pApi)
        {
            std::cout << "[Redis] Failed Get Api Data!!!" << std::endl;
            return;
        }
        if (status != REDIS_OK)
        {
            UT_LOG_ERROR_FMT(pApi->GetLogger(), "[Redis] OnConnect_Callback Error: %s", c->errstr);
            return;
        }
        // pick up the spi object pointer
        if (pApi->GetSpi())
            pApi->GetSpi()->OnConnected();
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::OnConnect_Callback exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::OnConnect_Callback Unknown Exception." << " \n";
    }
}

void CRedisApi::OnDisconnect_Callback(const redisAsyncContext *c, int status) 
{
    try
    {
        CRedisApi* pApi = static_cast<CRedisApi*>(c->data);
        if (!pApi)
        {
            std::cout << "[Redis] Failed Get Api Data!!!" << std::endl;
            return;
        }
        if (status != REDIS_OK) 
        {
            UT_LOG_ERROR_FMT(pApi->GetLogger(), "[Redis] OnDisconnect_Callback Error: %s", c->errstr);
            return;
        }
        if (pApi->GetSpi())
            pApi->GetSpi()->OnDisconnected(status);
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::OnDisconnect_Callback exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::OnDisconnect_Callback Unknown Exception." << " \n";
    }
}

void CRedisApi::SubscribeTopic(const std::string& topic)
{
    try
    {
        // InitAsyncRedisContext();
        // std::string cmd = "SUBSCRIBE " + topic;
        redisAsyncCommand(AsyncRedisContext, OnMessage_Callback, this, "SUBSCRIBE %s", topic.c_str());
        UT_LOG_INFO_FMT(Logger, "[Redis] SubscribeTopic: %s", topic.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::SubscribeTopic exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::SubscribeTopic Unknown Exception." << " \n";
    }
}

void CRedisApi::UnSubscribeTopic(const std::string& topic)
{
    try
    {
        // InitAsyncRedisContext();
        // std::string cmd = "SUBSCRIBE " + topic;
        redisAsyncCommand(AsyncRedisContext, OnMessage_Callback, this, "UNSUBSCRIBE %s", topic.c_str());
        UT_LOG_INFO_FMT(Logger, "[Redis] UnSubscribeTopic: %s", topic.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::UnSubscribeTopic exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::UnSubscribeTopic Unknown Exception." << " \n";
    }
}

void CRedisApi::PSubscribeTopic(const std::string& topic)
{
    try
    {
        // InitAsyncRedisContext();
        // std::string cmd = "SUBSCRIBE " + topic;
        redisAsyncCommand(AsyncRedisContext, OnMessage_Callback, this, "PSUBSCRIBE %s", topic.c_str());
        UT_LOG_INFO_FMT(Logger, "[Redis] PSubscribeTopic: %s", topic.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    catch (std::exception& e)
    {
        std::cerr << "CRedisApi::PSubscribeTopic exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "CRedisApi::PSubscribeTopic Unknown Exception." << " \n";
    }
}

void CRedisApi::PUnSubscribeTopic(const std::string& topic)
{
    try
    {
        // InitAsyncRedisContext();
        // std::string cmd = "SUBSCRIBE " + topic;
        redisAsyncCommand(AsyncRedisContext, OnMessage_Callback, this, "PUNSUBSCRIBE %s", topic.c_str());
        UT_LOG_INFO_FMT(Logger, "[Redis] PUnSubscribeTopic: %s", topic.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    catch (std::exception& e)
    {
        std::cerr << "CRedisApi::PUnSubscribeTopic exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "CRedisApi::PUnSubscribeTopic Unknown Exception." << " \n";
    }
}

// void CRedisApi::Publish(const string& channel, const string& content, const string& identity, bool aggregate/* =false */)
// {
//     std::unique_lock<std::mutex> lock(MutexPublish);
//     if (identity.empty())
//     {
//         UT_LOG_ERROR(Logger, "[Redis] Publish SessionID Empty: [Channel=" << channel << "] [Content=" << content << "] [SessionID=" << identity << "]");
//         return;
//     }
//     if (aggregate)
//     {
//         auto& publish_idx = PublishContent.get<idx_identity_channel>();
//         auto iter_publish = publish_idx.find(boost::make_tuple(identity, channel));
//         if (iter_publish != publish_idx.end())
//         {
//             publish_idx.erase(iter_publish);
//         }
//     }
//     PublishContent.emplace(identity, channel, content);
//     lock.unlock();
//     ContainerCodition.notify_one();
// }

// void CRedisApi::AsyncSet(const string& key, const string& value)
// {
//    try {
//        // InitRedisContext();
//        std::string cmd = "SET " + key + " " + value;
//        redisAsyncCommand(AsyncRedisContext, OnMessage_Callback, this, cmd.c_str());
//        UT_LOG_INFO_FMT(Logger, "[Redis] Set: %s", cmd.c_str());
//    }
//    catch (std::exception& e)
//    {
//        UT_LOG_ERROR(Logger, "[CRedisApi] SyncGet exception: " << e.what() << " " << key);
////        return "";
//    }
//    catch (...)
//    {
//        UT_LOG_ERROR(Logger, "[CRedisApi] SyncGet Unknow Exception: " << key);
////        return "";
//    }
// }
//
// void CRedisApi::AsyncGet(const string& key)
// {
//     // InitRedisContext();
//     std::string cmd = "GET " + key;
//     std::string *privdata = new std::string(key);
//     redisAsyncCommand(AsyncRedisContext, OnGetMessage_Callback, privdata, cmd.c_str());
//     UT_LOG_INFO_FMT(Logger, "[Redis] Get: %s", cmd.c_str());
// }

// void CRedisApi::Select(int database)
// {
//     // InitRedisContext();
//     char cmd[16]= {0};
//     sprintf(cmd, "%s %d", "SELECT", database);
//     redisAsyncCommand(RedisContext, OnGetMessage_Callback, this, cmd);
//     UT_LOG_INFO_FMT(Logger, "[Redis] Select: %s", cmd);
// }

std::string CRedisApi::SyncGet(const string& key)
{
    try
    {
        if (!syncontext_connected_) return "";
        std::string cmd = "GET " + key;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SyncGet %s error!!!", key.c_str());
            return "";
        }
        string result{nullptr == reply->str ? "" : reply->str};
        freeReplyObject(reply);
        return result;
    }
    catch (std::exception& e)
	{
        UT_LOG_ERROR(Logger, "[CRedisApi] SyncGet exception: " << e.what() << " " << key);
        return "";
	}
    catch (...)
    {
        UT_LOG_ERROR(Logger, "[CRedisApi] SyncGet Unknow Exception: " << key);
        return "";
    }
}

void CRedisApi::SyncSet(const string& key, const string& value){
    try
    {
        if (!syncontext_connected_) return ;
        std::string cmd = "SET " + key +" " +value;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SyncSet %s error!!!", key.c_str());
            return ;
        }
        string result{nullptr == reply->str ? "" : reply->str};
        freeReplyObject(reply);
        return ;
    }
    catch (std::exception& e)
    {
        UT_LOG_ERROR(Logger, "[CRedisApi] SyncSet exception: " << e.what() << " " << key);
        return ;
    }
    catch (...)
    {
        UT_LOG_ERROR(Logger, "[CRedisApi] SyncSet Unknown Exception: " << key);
        return ;
    }
}

list<string> CRedisApi::Keys(const string &pattern){
    list<string> ret;
    try
    {
        if (!syncontext_connected_) return ret;
        std::string cmd = "KEYS " + pattern;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "Keys %s error!!!", pattern.c_str());
            return ret;
        }
        for (size_t i=0;i<reply->elements; ++i){
            string result{reply->element[i]->str};
            ret.push_back(result);
        }
        freeReplyObject(reply);
        return ret;
    }
    catch (std::exception& e)
    {
        UT_LOG_ERROR(Logger, "[CRedisApi] Keys exception: " << e.what() << " " << pattern);
        return ret;
    }
    catch (...)
    {
        UT_LOG_ERROR(Logger, "[CRedisApi] Keys Unknow Exception: " << pattern);
        return ret;
    }
}

// hget & hset
string CRedisApi::HGet(const string& key, const string& field){
    string ret;
    try
    {
        if (!syncontext_connected_) return ret;
        std::string cmd = "HGet " + key + " " + field;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "HGet %s %s error!!!", key.c_str(), field.c_str());
            return ret;
        }
        string result{nullptr == reply->str ? "" : reply->str};
        freeReplyObject(reply);
        ret = result;
    } catch (...) {

    }
    return ret;
}

list<pair<string, string> > CRedisApi::HGetAll(const string& key){
    list<pair<string, string> > ret;
    try
    {
        if (!syncontext_connected_) return ret;
        std::string cmd = "HGetAll " + key ;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "HGetAll %s error!!!", key.c_str());
            return ret;
        }
        for (size_t i=0;i<reply->elements; i+=2){
            string key{reply->element[i]->str};
            string field{reply->element[i+1]->str};
            ret.push_back(make_pair(key, field));
        }
        freeReplyObject(reply);
    } catch (...) {

    }
    return ret;
}
void CRedisApi::HSet(const string& key, const string& field, const string& value){
    try
    {
        if (!syncontext_connected_) return ;
        std::string cmd = "HSet " + key + " " + field +" "+value;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "HSet %s %s error!!!", key.c_str(), field.c_str());
            return ;
        }

        freeReplyObject(reply);
        return ;
    } catch (...) {

    }
}
void CRedisApi::HDel(const string& key, const string& field){
    try
    {
        if (!syncontext_connected_) return ;
        std::string cmd = "HDel " + key + " " + field;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "HDel %s %s error!!!", key.c_str(), field.c_str());
            return ;
        }

        freeReplyObject(reply);
        return ;
    } catch (...) {

    }
}
// sadd & srem & spop(随机返回并删除名称为 key 的 set 中一个元素)
void CRedisApi::SAdd(const string &set, const string& member){
    try
    {
        if (!syncontext_connected_) return ;
        std::string cmd = "SAdd " + set + " " + member;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SAdd %s %s error!!!", set.c_str(), member.c_str());
            return ;
        }

        freeReplyObject(reply);
        return ;
    } catch (...) {

    }
}
void CRedisApi::SRem(const string &set, const string& member){
    try
    {
        if (!syncontext_connected_) return ;
        std::string cmd = "SRem " + set + " " + member;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SRem %s %s error!!!", set.c_str(), member.c_str());
            return ;
        }

        freeReplyObject(reply);
        return ;
    } catch (...) {

    }
}
//string CRedisApi::SPop(const string &set){
//
//}
set<string> CRedisApi::SMembers(const string &set){
    std::set<string> ret;
    try
    {
        if (!syncontext_connected_) return ret;
        std::string cmd = "SMembers " + set;
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SMembers %s error!!!", set.c_str());
            return ret;
        }
        for (size_t i=0;i<reply->elements; ++i){
            string result{reply->element[i]->str};
            ret.emplace(result);
        }
        freeReplyObject(reply);
    } catch(...){

    }
    return ret;
}

void CRedisApi::SyncSelect(int database)
{
    try
    {
        // InitSyncRedisContext();
        char cmd[16]= {0};
        sprintf(cmd, "%s %d", "SELECT", database);
        auto reply = (redisReply *)redisCommand(SyncRedisContext, cmd);
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            UT_LOG_ERROR_FMT(Logger, "SyncSelect %d error!!!", database);
            return ;
        }
        UT_LOG_INFO_FMT(Logger, "[Redis] Select: %s", cmd);
    }
    catch (std::exception& e)
	{
		std::cerr << "CRedisApi::SyncSelect exception: " << e.what() << "\n";
	}
    catch (...)
    {
        std::cerr << "CRedisApi::SyncSelect Unknown Exception: " << database << " \n";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRedisApiPublish::CRedisApiPublish(CRedisApi::UTLogPtr logger, io_service_pool& pool) : CRedisApi{logger}, ThreadBasePool{pool}
{

}

CRedisApiPublish::~CRedisApiPublish()
{

}

// publish the event
void CRedisApiPublish::Publish(boost::shared_ptr<string> channel, boost::shared_ptr<string> content)
{
    get_io_service().post(std::bind(&CRedisApiPublish::HandlePublish, this, channel, content));
}

void CRedisApiPublish::HandlePublish(boost::shared_ptr<string> channel, boost::shared_ptr<string> content)
{
    try
    {
        void* reply = redisCommand(SyncRedisContext, "PUBLISH %s %s", channel->c_str(), content->c_str());
        freeReplyObject(reply);
        // UT_LOG_INFO_FMT(Logger, "[Redis] PublishTopic: %s %s", channel->c_str(), content->c_str());
    }
    catch (std::exception& e)
    {
        std::cerr << "CRedisApi::Publish exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "CRedisApi::Publish Unknown Exception." << " \n";
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void CRedisApi::PublishRun()
// {
//     void* reply;
//     while(true)
//     {
//         std::unique_lock<std::mutex> lock{MutexPublish};
//         // 如果后面的条件不满足则阻塞，等待　wakeup　通知，再检测条件是否满足，不满足接着阻塞，注意判断退出情况
//         ContainerCodition.wait(lock, [this](){ return StopPublish.load() || !PublishContent.empty(); });
//         // 如果只是要关闭线程则直接退出
//         if (StopPublish.load()){ return; }

//         // pick up element to send
//         auto& publish_idx = PublishContent.get<idx_publish_sequence>();
//         int handle_count{0};
//         while (handle_count<HandleCountOnce && !PublishContent.empty())
//         {
//             auto iter_publish = publish_idx.begin();
//             PublishProperty publish_property{std::move(*iter_publish)};
//             publish_idx.erase(iter_publish);
//             lock.unlock();
//             try
//             {
//                 reply = redisCommand(SyncRedisContext, "PUBLISH %s %s", publish_property.Channel.c_str(), publish_property.Message.c_str());
//                 freeReplyObject(reply);
//                 // UT_LOG_INFO_FMT(Logger, "[Redis] PublishTopic: %s %s", publish_property.Channel.c_str(), publish_property.Message.c_str());
//             }
//             catch (std::exception& e)
//             {
//                 std::cerr << "CRedisApi::Publish exception: " << e.what() << "\n";
//             }
//             lock.lock();
//         }
//     }
// }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
class Receiver : public CRedisSpi
{
    // redis connect notify
    virtual void OnConnected()
    {
        std::cout << "on connected!" << std::endl;
    }
    // redis disconnect notify
    virtual void OnDisconnected(int status)
    {
        std::cout << "on disconnected" << std::endl;
    }
    // redis message notify
    virtual void OnMessage(const std::string& channel, const std::string& msg)
    {
        std::cout << channel << " " << msg << std::endl;
    }
    // redis get message notify
    virtual void OnGetMessage(const std::string& key, const std::string& value)
    {
        std::cout << key << " " << value << std::endl;
    }
};


vector<string> split(const string& src,const string& delim) 
{
    vector<string> des;  
    if("" == src) return  des;  
      
    string tmpstrs = src + delim;
    size_t pos;  
    size_t size = tmpstrs.size();  
  
    for (size_t i = 0; i < size; ++i) {  
        pos = tmpstrs.find(delim, i); 
        if( pos < size) {
            des.push_back(tmpstrs.substr(i, pos - i)); 
            i = pos + delim.size() - 1;  
        }  
    }  
    return des;   
}  

/*
int main(int argc, char const *argv[])
{
    UTLogPtr logger = UTLog::getStrategyLogger("test_redis", "test_redis");
    Receiver recv;
    CRedisApi api(logger);
    api.RegisterRedis("128.1.135.86", 9418, "hk_develop_1");
    api.RegisterSpi(&recv);

    //select database
    // api.Select(4);

    // api.Set("hk", "we");  
    // api.Get("hk");
    
    // api.SubscribeTopic("test_channel");
    // api.Publish("test_channel", "{\"id\":1001}");

    // api.Publish("test_channel", "{\"id\":1001}");
    // api.Publish("test_channel", "{\"id\":1001}");
    // api.Publish("test_channel", "{\"id\":1001}");
    // api.Set("hk", "sdfwe");  
    // api.Get("hk");
    // api.Set("hk", "sdf343w4535we");  
    // api.Get("hk");

    api.SyncSelect(1);

    // string risk_relation = api.SyncGet("risk_relation");
    // vector<string> relations = split(risk_relation, "&");
    // for (auto relation : relations)
    // {
    //     vector<string> items = split(relation, ",");
    //     for (auto item : items)
    //     {
    //         std::cout << item << "  ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
    // string risk_setting = api.SyncGet("risk_setting");
    // vector<string> settings = split(risk_setting, "&");
    // for (auto setting : settings)
    // {
    //     vector<string> items = split(setting, ",");
    //     for (auto item : items)
    //     {
    //         std::cout << item << "  ";
    //     }
    //     std::cout << std::endl;
    // }




    string enc_raw_info = api.SyncGet("raw_info");
    auto infos = split(enc_raw_info, "&");
    string raw_info = "";

    FILE *private_key_file = fopen("./private.pem", "rb");
    RSA* rsa = RSA_new();
    auto read_ret = PEM_read_RSAPrivateKey(private_key_file, &rsa, 0, 0);
    if (read_ret == nullptr) std::cout << "PEM_read_RSAPrivateKey error!\n";
    int rsa_len = RSA_size(rsa);
    for (auto info : infos)
    {
        char *decode_info = new char[rsa_len + 1];
        memset(decode_info, 0, rsa_len + 1);
        int info_len = info.length();
        char *buffer = new char[info_len];
        memset(buffer, 0, info_len);
        
        BIO *b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO *bmem = BIO_new_mem_buf(info.c_str(), info_len);
        bmem = BIO_push(b64, bmem);
        int read_len = BIO_read(bmem, buffer, info_len);
        BIO_free_all(bmem);

        auto ret = RSA_private_decrypt(read_len, (const unsigned char*)buffer, (unsigned char*)decode_info, rsa, RSA_PKCS1_PADDING);
        if (ret > 0)
        {
            raw_info = raw_info + decode_info;
        }
        delete[] decode_info;
        delete[] buffer;

    }
    RSA_free(rsa);
    fclose(private_key_file);
    CRYPTO_cleanup_all_ex_data();
    std::cout << raw_info << std::endl;



    // api.Start();

    // while(true) usleep(1000000000);
    return 0;
}
*/