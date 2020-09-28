#pragma once
#include "../envdeclare.h"
#include <sys/time.h>
#include "channel.h"
#include "../util/time_util.h"

// message chan flag
const char CHAIN_CONTINUE	    = 'C';		//　not last package
const char CHAIN_LAST		    = 'L';		// last package

const unsigned char UT_REQUEST	 = 'R';		// 请求
const unsigned char UT_RESPONSE	 = 'O';		// 应答
const unsigned char UT_SUBSCRIBE = 'S';		// 订阅
const unsigned char UT_PUBLISH	 = 'P';		// 发布
const unsigned char UT_UNKNOWN	 = 'U';		// 未知

struct PackageHead
{
    PackageHead() {}
    // package session identity
    std::string SessionID{""};
    // package type (request, response or publish)
    unsigned char Type{UT_UNKNOWN};
    // message id
    unsigned int Tid{0};
    // chain last?
    unsigned char	Chain{CHAIN_LAST};
    // RequestID
    long RequestID{0};
    // sequence number
    long SequenceNo{0};
    // Package ID（全局的数据包编号）
    long PackageID{0};
    // channel name
    std::string Channel{""};
    // package create time
    long CreateTime{0};
    // physical name
    std::string Physical{""};
    // access token (TODO: to remove when we don't use redis to send data)
    std::string AccessToken{""};
};

// specially for handle with package head
struct PackageBase
{
    PackageBase()
    {
        Head.CreateTime = utrade::pandora::NanoTime();
        // std::cout << "init package time " << Head.CreateTime << std::endl;
    }
    virtual ~PackageBase(){}

    void prepare_request(const unsigned int tid, long request_id, unsigned char chain=CHAIN_LAST, const std::string& sessionid="")
    {
        Head.SessionID = sessionid;
        Head.Type = UT_REQUEST;
        Head.Tid = tid;
        Head.Chain = chain;
        Head.RequestID = request_id;
    }

    void prepare_response(const unsigned int tid, long request_id, unsigned char chain=CHAIN_LAST, const std::string& sessionid="")
    {
        Head.SessionID = sessionid;
        Head.Type = UT_RESPONSE;
        Head.Tid = tid;
        Head.Chain = chain;
        Head.RequestID = request_id;
    }

    void prepare_response(PackageBase* package, const unsigned int tid)
    {
        Head.SessionID = package->SessionID();
        Head.Type = UT_RESPONSE;
        Head.Tid = tid;
        Head.Chain = CHAIN_LAST;
        Head.RequestID = package->RequestID();
        Head.Channel = package->Channel();
    }

    void prepare_subscribe(const unsigned int tid, const std::string& sessionid="")
    {
        Head.SessionID = sessionid;
        Head.Type = UT_SUBSCRIBE;
        Head.Tid = tid;
        Head.Chain = CHAIN_LAST;
    }

    void prepare_publish(const unsigned int tid, unsigned char chain=CHAIN_LAST, const std::string& sessionid="")
    {
        Head.SessionID = sessionid;
        Head.Type = UT_PUBLISH;
        Head.Tid = tid;
        Head.Chain = chain;
        Head.RequestID = 0;
        Head.SequenceNo = 0;
        Head.Channel = "";
    }

    void send_head(boost::shared_ptr<IChannel> sender, bool pack_session_id, bool is_last=false)
    {
        if (pack_session_id)
            sender->send(Head.SessionID, is_last);
        sender->send(Head.Tid, is_last);
        sender->send(Head.Type, is_last);
        sender->send(Head.Chain==CHAIN_LAST?1:0, is_last);
        sender->send(Head.RequestID, is_last);
        sender->send(Head.SequenceNo, is_last);
        sender->send(Head.CreateTime, is_last);
        sender->send(Head.Channel, is_last);
        // std::cout << "[SEND_HEAD] [SessionID=" << Head.SessionID << "] [Tid=" << Head.Tid << "] [RequestID=" << Head.RequestID << "] [Chain=" << Head.Chain << "] ";
    }

    void read_head(boost::shared_ptr<IChannel> reader, bool pack_session_id)
    {
        if (pack_session_id)
            reader->read(Head.SessionID);
        reader->read(Head.Tid);
        reader->read(Head.Type);
        std::string chain_sign;
        reader->read(chain_sign);
        Head.Chain = chain_sign=="1" ? CHAIN_LAST : CHAIN_CONTINUE;
        reader->read(Head.RequestID);
        reader->read(Head.SequenceNo);
        reader->read(Head.CreateTime);
        reader->read(Head.Channel);
        // std::cout << "[READ_HEAD] [SessionID=" << Head.SessionID << "] [Tid=" << Head.Tid << "] [RequestID=" << Head.RequestID << "] [Chain=" << Head.Chain << "] ";
    }

    // this package is request package ?
    bool is_request_package()
    {
        return Head.Type == UT_REQUEST;
    }

    // retrieve the current time
    string time_now()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        auto tm_time = gmtime(&tv.tv_sec);
        char now[32] = {0};
        sprintf(now, "%04d-%02d-%02d %02d:%02d:%02d.%6ld", tm_time->tm_year + 1900, tm_time->tm_mon+1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, tv.tv_usec);
        return now;
    }

    // head str
    std::string head_str()
    {
        std::string head;
        head += "[SessionID=" + Head.SessionID + "] ";
        head += "[Type=" + std::to_string(Head.Type) + "] ";
        char tid[12]={'\0'}; sprintf(tid, "0x%08x", Head.Tid);
        head += "[Tid=" + std::string(tid) + "] ";
        head += "[Chain=" + std::to_string(Head.Chain) + "] ";
        head += "[RequestID=" + std::to_string(Head.RequestID) + "] ";
        head += "[SequenceNo=" + std::to_string(Head.SequenceNo) + "] ";
        head += "[Channel=" + Head.Channel + "] ";
        head += "[CreateTime=" + std::to_string(Head.CreateTime) + "] ";
        head += "[PhysicalName=" + Head.Physical + "] ";
        return head;
    }

    // retrieve the message type
    unsigned int Tid(){ return Head.Tid; }
    // retrieve the strategy name
    std::string  SessionID(){ return Head.SessionID; }
    // retrieve the request identity number
    long         RequestID(){ return Head.RequestID; }
    // is the last response
    bool         IsLast(){ return Head.Chain==CHAIN_LAST; }
    // chain
    unsigned char Chain() { return Head.Chain; }
    // type
    unsigned char Type(){ return Head.Type; }
    // SequenceNo
    long SequenceNo(){ return Head.SequenceNo; }
    // channel
    std::string Channel() { return Head.Channel; }
    // physical account name
    std::string Physical() { return Head.Physical; }
    // create time
    long CreateTime() { return Head.CreateTime; }
    // access token information
    std::string AccessToken() { return Head.AccessToken; }
    // package ID
    long PackageID() { return Head.PackageID; }

    // set request id
    void SetRequestID(long request_id) { Head.RequestID=request_id; }
    // set channel field
    void SetChannel(const std::string& channel) { Head.Channel = channel; }
    // set sequence number
    void SetSequenceNo(long sequenceNo) { Head.SequenceNo=sequenceNo; }
    // set physical account name
    void SetPhysical(const string& physical) { Head.Physical=physical; }
    // reset Create Time
    void SetCreateTime(const long& create_time) { Head.CreateTime = create_time; }
    // set Access Token
    void SetAccessToken(const std::string& access_token) { Head.AccessToken=access_token; }
    // set the session identity
    void SetSessionID(const std::string& session_id) { Head.SessionID=session_id; }
    // global package id
    void SetPackageID(const long& package_id) { Head.PackageID=package_id; }
    // reset the package
    virtual void reset_fields() = 0;
    // field head struct
    PackageHead Head;
};

class FieldIterator
{
public:
    FieldIterator(std::vector<FieldDef>& field, long& fid) : seek_index_(0), fid_iter_(fid), fields_(field)
    {

    }

    void* next()
    {
        // memcpy(data, fields_[seek_index_].Field.get(), fields_[seek_index_].FieldLength);
        void* data = fields_[seek_index_].Field.get();
        ++seek_index_;
        return data;
    }

    bool more()
    {
        while (seek_index_<fields_.size())
        {
            if (fid_iter_<0)
                return true;
            else
            {
                if (fields_[seek_index_].FieldID==fid_iter_)
                    return true;
                ++seek_index_;
            }
        }
        return false;
    }

    // seek index
    size_t seek_index_;
    // the fid that we need to iter, if -1, any field return 
    long fid_iter_;
    // the fields
    std::vector<FieldDef>& fields_;
};

struct Package : public PackageBase
{
    Package()
    {
        reset_fields();
    }
    
    virtual ~Package()
    {
        reset_fields();
    }

    virtual void reset_fields()
    {
        // std::cout << "reset_fields data" << std::endl;
        Fields.clear();
    }

    void send_package(boost::shared_ptr<IChannel> sender)
    {
        send_head(sender, pack_session_id);
        sender->send(Fields);
        // std::cout << " | [SEND_DATA] ";
        // for (auto& i : Fields)
        //     std::cout << i.FieldID << ":" << i.FieldLength << std::endl;
        // std::cout << std::endl;
        // clear the buffer
        // reset_fields();
    }

    void read_package(boost::shared_ptr<IChannel> reader)
    {
        // before we start, clear the buffer
        reset_fields();
        read_head(reader, pack_session_id);
        reader->read(Fields);
        // std::cout << " | [READ_DATA]";
        // for (auto& i : Fields)
        //     std::cout << i.FieldID << ":" << i.FieldLength << std::endl;
        // std::cout << std::endl;
    }

    // create field
    // cause we can't copy data too many time, so we get the allocate memory from deeper inside
    void* create_field(int len, long fid)
    {
        auto it = std::find(Fields.begin(), Fields.end(), FieldDef(len,fid));
        if(it != Fields.end())
        {
            return get_field(fid);
        }
        Fields.emplace_back(len, fid);
        return get_field(fid);

    }

    // get field
    void* get_field(long fid)
    {
        FieldIterator it = iterator(fid);
        while (it.more())
        {
            return it.next();
        }
        return nullptr;
    }

    FieldIterator iterator(long fid=-1)
    {
        return FieldIterator(Fields, fid);
    }

    // if we need send session id, this is very import
    // when we in platform, we can't judge which strategy to send and we should set the session id
    // but in strategy we know who I am, and when send message zmq add the identidy automatically, we don't add manuanlly.
    static bool pack_session_id;
    // all fields pack here
    std::vector<FieldDef> Fields;
};

DECLARE_PTR(Package);

#define CREATE_FIELD(pack, cls)\
    static_cast<cls*>((pack)->create_field(sizeof(cls), cls::Fid))

#define GET_FIELD(pack, cls)\
    static_cast<const cls*>((pack)->get_field(cls::Fid))

#define GET_NON_CONST_FIELD(pack, cls)\
    static_cast<cls*>((pack)->get_field(cls::Fid))

#define SEND_PACKAGE(pack, sender)\
    (pack)->send_package(sender)

#define READ_PACKAGE(pack, reader)\
    (pack)->read_package(reader)

#define SEND_PACKAGE_BY_AGENT(pack, sender_agent)\
    (pack)->send_package(sender_agent->sender())

