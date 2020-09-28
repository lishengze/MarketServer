/*
 * @Author: haiping.zeng
 * @Date: 2018-11-30
 * @des: this file used for send message to dingtalk
 * 
 * @Last Modified by: daniel.bian
 * @Last Modified Date: 2018-12-03
 * @des: simplize the process to send message
 */

#pragma once
#include "../pandora_declare.h"
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "../util/thread_basepool.h"

PANDORA_NAMESPACE_START

// ding talk message body definition
struct DingMessageBody
{
    DingMessageBody(const string& title, const string& level, int error_id, const string& error_msg, bool at_all, const string& time) :
                    Title{title}, Level{level}, ErrorID{error_id}, ErrorMsg{error_msg}, AtAll{at_all}, UpdateTime{time}
    {}
    string Title{""};       // message title
    string Level{""};       // message level
    int ErrorID{0};         // message error id
    string ErrorMsg{""};    // message error message
    bool AtAll{false};      // send message to all user in the group
    string UpdateTime{""};  // this type of message last update time
    int OccurTimes{1};      // this type of message occur times,  we merge the same type message together.
};

DECLARE_PTR(DingMessageBody);

// ding talk 
class DingTalk : public ThreadBasePool
{
    using asyn_headers_t = boost::shared_ptr<vector<string>>;
public:
    DingTalk(io_service_pool& pool, const std::vector<std::string>& addresses);
    virtual ~DingTalk();
    // send message to ding talk
    void send_message(const char* title, const char* level, int error_id, const char* errorMsg, bool at_all=false);
    void send_message(const string& title, const string& level, int error_id, const string& errorMsg, bool at_all=false);
    // // stop running
    // void stop() { stop_thread_.store(true);}
    // // running the thread
    // void run();

private:
    // on timer event
    void on_timer(int millisecond);
    // handle send message
    void handle_send_message(DingMessageBodyPtr message);
    // request done notification
    static size_t http_request_done(void *msg, size_t size, size_t nmemb, void *stream);
    // puck one robot url
    boost::shared_ptr<std::string> pick_url();
    // handle send messages
    void execute_send_message(boost::shared_ptr<std::string> url, boost::shared_ptr<std::string> data, asyn_headers_t headers);
    // get time now
    std::string time_now();
    // process the message
    void process_message();
    // the ding talk robot address collection
    std::vector<std::string> robot_address_;
    // robot iterator
    std::vector<std::string>::iterator iter_robot_;
    // messager queue
//    std::unordered_map<string, DingMessageBodyPtr> ding_message_queue_;
    std::unordered_map<string, long> ding_message_occur_times_;
    std::list<DingMessageBodyPtr> ding_message_queue_;
    // // lock object
    // std::mutex mutex_queue_;
    // stop the ding talker
    bool running_{true};
    // // condition variable
	// std::condition_variable queue_condition_;
    // // stop thread
	// std::atomic<bool> stop_thread_{false};
    // // thread to publish ding message
    // ThreadPtr ding_thread_;
    // timer for call
    boost::shared_ptr<boost::asio::deadline_timer> timer_;
};

DECLARE_PTR(DingTalk);

PANDORA_NAMESPACE_END