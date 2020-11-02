#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
using namespace std;

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
using grpc::Alarm;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;


class CommonGrpcCall
{
public:
    virtual void on_connect(void*) = 0;
    virtual void on_disconnect(void*) = 0;
    virtual void release(void*) = 0;
    virtual void process(void*) = 0;
    virtual void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) = 0;
};

class BaseGrpcEntity
{
public:
    bool is_first;
    grpc::Alarm alarm_;
    
    enum CallStatus { PROCESS, PUSH_TO_BACK, FINISH };
    CallStatus status_;  // The current serving state.

    CommonGrpcCall* caller_;

    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_ = nullptr;

    int call_id_;
public:
    virtual void register_call() = 0;
    virtual bool process() = 0;
    virtual void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) = 0;

    BaseGrpcEntity():is_first(true),status_(PROCESS),caller_(NULL),cq_(NULL),call_id_(-1){}
    virtual ~BaseGrpcEntity(){}

    void set_callid(int call_id) { call_id_ = call_id; }
    void set_completequeue(ServerCompletionQueue* cq) { cq_ = cq; }
    void set_parent(CommonGrpcCall* caller) { caller_ = caller; }
    
    void release() {
        caller_->on_disconnect(this);
        delete this;
    }

    void proceed() { 
        if( status_ == PROCESS) {
            if ( is_first )
            {
                is_first = false;
                caller_->on_connect(this);
            }
            
            // process返回true表示有消息发送，返回false表示无消息，需要手动插入一个事件
            if( !process() ) {
                alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
            }

            // 
            if( status_ != FINISH )
                status_ = PUSH_TO_BACK;
        } else if(status_ == PUSH_TO_BACK) {
            status_ = PROCESS;
            alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
        } else {
            caller_->on_disconnect(this);
            GPR_ASSERT(status_ == FINISH);
            delete this;
        }
    }
};

template<class ENTITY>
class GrpcCall : public CommonGrpcCall
{
public:
    GrpcCall(int call_id, void* service, ServerCompletionQueue* cq): call_id_(call_id), service_(service), cq_(cq)
    {
        _register();
    }

    void release(void* entity) {
        ((ENTITY*)entity)->release();
    }

    void process(void* entity) {
        ((ENTITY*)entity)->proceed();
    }

    void on_connect(void* entity) {
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
            clients_.insert((ENTITY*)entity);
        }
        _register();
    }

    void on_disconnect(void* entity) {
        std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
        auto iter = clients_.find(((ENTITY*)entity));
        if( iter != clients_.end() ) {
            clients_.erase(iter);
        }
    }

    void add_data(std::shared_ptr<void> snap, std::shared_ptr<void> update) {
        std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
        for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
            (*iter)->add_data(snap, update);
        }
    }

private:
    int call_id_;
    void* service_;
    ServerCompletionQueue* cq_ = nullptr;

    mutable std::mutex mutex_clients_;
    std::unordered_set<ENTITY*> clients_;

    void _register() {
        ENTITY* ptr = new ENTITY(service_);
        ptr->set_callid(call_id_);
        ptr->set_completequeue(cq_);
        ptr->set_parent(this);
        ptr->register_call();
    }
};
