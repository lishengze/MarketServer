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
#include "base/cpp/basic.h"


class CommonGrpcCall
{
public:
    virtual void on_connect(void*) = 0;
    virtual void on_disconnect(void*) = 0;
};

class BaseGrpcEntity
{
public:
    std::string entity_name_;
    bool is_first;
    grpc::Alarm alarm_;
    
    enum CallStatus { PROCESS, PUSH_TO_BACK, FINISH };
    CallStatus status_;  // The current serving state.

    CommonGrpcCall* caller_;

    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_ = nullptr;

    int call_id_;

    ServerContext ctx_;

    int64 current_loop_id_;

    bool is_active_;
    bool is_processing_;

    bool is_inner_write_{false};
public:
    virtual void register_call() = 0;
    virtual bool process() = 0;
    virtual void on_init() {};
    virtual string get_entity_name() {return entity_name_; }

    BaseGrpcEntity(std::string entity_name="BaseGrpcEntity"):entity_name_{entity_name},is_first(true),status_(PROCESS),caller_(NULL),cq_(NULL),call_id_(-1),is_active_(true),is_processing_(false){}
    virtual ~BaseGrpcEntity(){}

    void make_active() {
        // cout << "make_active: " << get_entity_name() << endl;
        if( !is_processing_ && !is_active_ ) {
            is_active_ = true;
            gpr_timespec t = gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME);
            //tfm::printfln("make_active %u.%u", t.tv_sec, t.tv_nsec);
            alarm_.Set(cq_, t, this);
        }
    }

    ServerContext* get_context() { return &ctx_; }

    void set_callid(int call_id) { call_id_ = call_id; }
    void set_completequeue(ServerCompletionQueue* cq) { cq_ = cq; }
    void set_parent(CommonGrpcCall* caller) { caller_ = caller; }
    
    void release(int64& loop_id) {
        current_loop_id_ = loop_id ++;

        caller_->on_disconnect(this);
        delete this;
    }

    void proceed(int64& loop_id) { 
        current_loop_id_ = loop_id ++;

        if( status_ == PROCESS) {
            if ( is_first )
            {
                is_first = false;
                caller_->on_connect(this);
            }
            
            // process返回true表示有消息发送
            // 返回false表示无消息，需要手动插入一个事件
            {                
                TimeCostWatcher w("_handle_rpcs");
                is_processing_ = true;
                if( !process() ) {
                    is_active_ = false;
                    //alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
                }
                is_processing_ = false;
            }

            //if( status_ != FINISH )
            //    status_ = PUSH_TO_BACK;
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
    template<class ...Args>
    GrpcCall(int& call_id, ::grpc::Service* service, ServerCompletionQueue* cq, Args... rest): call_id_(call_id), service_(service), cq_(cq)
    {
        ENTITY* ptr = new ENTITY(service, rest...);
        ptr->set_callid(call_id);
        ptr->set_completequeue(cq);
        ptr->set_parent(this);
        ptr->register_call();
        call_id++;
    }

    void on_connect(void* entity) {
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
            clients_.insert((ENTITY*)entity);
        }
        ((ENTITY*)entity)->on_init();

        ENTITY* ptr = ((ENTITY*)entity)->spawn();
        //ENTITY* ptr = new ENTITY(service_);
        ptr->set_callid(call_id_);
        ptr->set_completequeue(cq_);
        ptr->set_parent(this);
        ptr->register_call();
    }

    void on_disconnect(void* entity) {
        std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
        auto iter = clients_.find(((ENTITY*)entity));
        if( iter != clients_.end() ) {
            clients_.erase(iter);
        }
    }

    template<class ...Args>
    void add_data(Args... rest) {
        {
            std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
            for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {

                // cout << "\n" << (*iter)->get_entity_name() << " out add data" << endl; 
                (*iter)->make_active();

                (*iter)->add_data(rest...);
            }
        }
    }
private:
    int call_id_;
    ::grpc::Service* service_;
    ServerCompletionQueue* cq_ = nullptr;

    mutable std::mutex mutex_clients_;
    std::unordered_set<ENTITY*> clients_;
};
