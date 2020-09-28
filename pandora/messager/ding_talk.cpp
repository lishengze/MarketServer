#include "ding_talk.h"
#include <sys/time.h>
#include "curl/curl.h"
#include "../util/time_util.h"
#include "../util/json.hpp"

USING_PANDORA_NAMESPACE
using namespace std;

DingTalk::DingTalk(utrade::pandora::io_service_pool& pool, const std::vector<std::string>& addresses) : ThreadBasePool{pool}, timer_{new boost::asio::deadline_timer{get_io_service()}}
{
    // init the robot address
    robot_address_.assign(addresses.begin(), addresses.end());

    // init the iterator
    iter_robot_ = robot_address_.begin();
    // start the thread 
    // ding_thread_ = ThreadPtr{new std::thread{&DingTalk::run, this}};
    get_io_service().post(std::bind(&DingTalk::on_timer, this, 1000));
    // on_timer(1000);
}

DingTalk::~DingTalk()
{
    // stop_thread_.store(true);
    // stop();
}

void DingTalk::on_timer(int millisecond)
{
    process_message();
    // timer invoke some microseconds later
    timer_->expires_from_now(boost::posix_time::milliseconds(millisecond));
    // call the asys call function
    timer_->async_wait(boost::bind(&DingTalk::on_timer, this, millisecond));
}

boost::shared_ptr<string> DingTalk::pick_url()
{
    // no robot address
    if (robot_address_.empty()) return boost::shared_ptr<string>{};
    if (iter_robot_!=robot_address_.end())
    {
        return boost::shared_ptr<string>{new string{*iter_robot_++}};
    }
    else
    {
        iter_robot_ = robot_address_.begin();
        return boost::shared_ptr<string>{new string{*iter_robot_++}};
    }
    // long time_current = NanoTime();
    // for (size_t index=0; index!=robot_address_.size(); ++index)
    // {
    //     if (iter_robot_==robot_address_.end())  iter_robot_=robot_address_.begin();
    //     if (time_current-iter_robot_->second >= 3*NANOSECONDS_PER_SECOND)
    //     {
    //         iter_robot_->second = time_current;
    //         return boost::shared_ptr<string>{new string{iter_robot_++->first}};
    //     }
    //     ++iter_robot_;
    // }
    // // return the nullptr
    // return boost::shared_ptr<string>{};
}

string DingTalk::time_now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    auto tm_time = gmtime(&tv.tv_sec);
    char now[32] = {0};
    sprintf(now, "%04d-%02d-%02d %02d:%02d:%02d.%6ld", tm_time->tm_year + 1900, tm_time->tm_mon+1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, tv.tv_usec);
    return now;
}

void DingTalk::send_message(const char* title, const char* level, int error_id, const char* errorMsg, bool at_all)
{
    send_message(string(title), string(level), error_id, string(errorMsg), at_all);
}

void DingTalk::send_message(const string& title, const string& level, int error_id, const string& errorMsg, bool at_all/* =false */)
{
    auto message = DingMessageBodyPtr{new DingMessageBody{title, level, error_id, errorMsg, at_all, time_now()}};
    get_io_service().post(std::bind(&DingTalk::handle_send_message, this, message));
}

void DingTalk::handle_send_message(DingMessageBodyPtr message)
{
    string message_key{message->Title + to_string(message->ErrorID)};
    //wang.hy level是Info，如果只是普通的通知消息，则都会发送，不合并
//    if(!message->Level.compare("Info"))
//    {
//        message_key = utrade::pandora::NanoTimeStr();
//    }
    auto it_index = ding_message_occur_times_.find(message_key);
    if(it_index != ding_message_occur_times_.end())
    {
        message->OccurTimes = ++it_index->second;
    }
    else
    {
        ding_message_occur_times_.emplace(message_key,1);
    }
    ding_message_queue_.push_back(message);

//    auto iter_find_message = ding_message_queue_.find(message_key);
//    if (iter_find_message!=ding_message_queue_.end())
//    {
//        iter_find_message->second->OccurTimes += 1;
//        iter_find_message->second->UpdateTime = time_now();
//    }
//    else
//    {
//        ding_message_queue_.emplace(message_key, message);
//    }
}

void DingTalk::process_message()
{
    if (ding_message_queue_.empty())    return;
    auto iter_message = ding_message_queue_.begin();
    //DingMessageBodyPtr message_body{iter_message->second};
    DingMessageBodyPtr message_body{*iter_message};
    ding_message_queue_.erase(iter_message);
    
    // std::cout << "start prepare message" << std::endl;
    // exclude the send dingtalk process, consider that we can't block any ding message during put in the queue
    // organize the message correct format to ding talk
    auto isAtAll = message_body->AtAll ? "true" : "false";
    stringstream sstream;
    sstream << "{\"msgtype\":\"markdown\", \"markdown\":";
    sstream << "{\"title\":\"" << message_body->Title << "\", \"text\":\"";
    sstream << "# **" << message_body->Title << "** \n";
    sstream << "### Level - **" << message_body->Level << "** \n";
    sstream << "### Error - **" << to_string(message_body->ErrorID) << "** \n";
    sstream << "### OccurTimes - **" << message_body->OccurTimes << "** \n";
    sstream << "##### " << message_body->ErrorMsg << "\n\n";
    sstream << "###### " << message_body->UpdateTime << "\"}";
    sstream << ", \"at\":{\"isAtAll\":" << isAtAll << "}}";
    auto data_body = boost::shared_ptr<string>{new string{sstream.str()}};
    // std::cout << *data_body.get() << std::endl;

    auto headers = asyn_headers_t{new vector<string>{}};
    (*headers).push_back("Content-Type: application/json; charset=utf-8");
    boost::shared_ptr<string> url{pick_url()};
    while (!url.get())
    {
        sleep(1);
        url = pick_url();
    }

    // std::cout << "url: " << *url.get() << std::endl;
    // handle send message
    //cout<<message_body->ErrorMsg<<endl;
    execute_send_message(url, data_body, headers);
}

// void DingTalk::run()
// {
//     while (running_)
//     {
//         std::unique_lock<std::mutex> lock{ mutex_queue_ };
//         // 如果后面的条件不满足则阻塞，等待wakeup通知，再检测条件是否满足，不满足接着阻塞
//         queue_condition_.wait(lock, [this](){ return stop_thread_.load() || !ding_message_queue_.empty(); });    // wait 直到有 task
//         // 如果只是要关闭线程则直接退出
//         if (stop_thread_.load()){ return; }
//         auto iter_message = ding_message_queue_.begin();
//         DingMessageBodyPtr message_body{iter_message->second};
//         ding_message_queue_.erase(iter_message);
//         // unlock the lock
//         lock.unlock();
        
//         // std::cout << "start prepare message" << std::endl;
//         // exclude the send dingtalk process, consider that we can't block any ding message during put in the queue
//         // organize the message correct format to ding talk
//         auto isAtAll = message_body->AtAll ? "true" : "false";
//         stringstream sstream;
//         sstream << "{\"msgtype\":\"markdown\", \"markdown\":";
//         sstream << "{\"title\":\"" << message_body->Title << "\", \"text\":\"";
//         sstream << "## **" << message_body->Title << "** \n";
//         sstream << "#### Level: " << message_body->Level << "\n";
//         sstream << "#### OccurTimes: " << message_body->OccurTimes << "\n";
//         sstream << ">### **Error - " << to_string(message_body->ErrorID) << "** \n";
//         sstream << ">##### " << message_body->ErrorMsg << "\n\n";
//         sstream << "###### " << message_body->UpdateTime << "\"}";
//         sstream << ", \"at\":{\"isAtAll\":" << isAtAll << "}}";
//         auto data_body = boost::shared_ptr<string>{new string{sstream.str()}};
//         // std::cout << *data_body.get() << std::endl;

//         auto headers = asyn_headers_t{new vector<string>{}};
//         (*headers).push_back("Content-Type: application/json; charset=utf-8");
//         boost::shared_ptr<string> url{pick_url()};
//         while (!url.get())
//         {
//             sleep(1);
//             url = pick_url();
//         }

//         // std::cout << "url: " << *url.get() << std::endl;
//         // handle send message
//         execute_send_message(url, data_body, headers);
//     }
// }

using json = nlohmann::json;
size_t DingTalk::http_request_done(void *msg, size_t size, size_t nmemb, void *stream)
{
    try
    {
        const char *str = (char *)msg;
        if (str[0] != '<')
        {
            json js = json::parse(string((char*)msg));
            if (js["errcode"].get<int>() != 0)
                cout << "DingTalk::http_request_done: " << (char *)msg << endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(...)
    {
        std::cerr << "------> http_request_done caught exception! ";
    }
    
    
    return size * nmemb;
}


void DingTalk::execute_send_message(boost::shared_ptr<string> url, boost::shared_ptr<string> data, asyn_headers_t headers)
{
    try
    {
        CURL *curl = curl_easy_init();
        CURLcode res;
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url->c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1);

            //set header
            struct curl_slist *curl_headers = nullptr;
            for (size_t i = 0; i < headers->size(); i++)
            {
                curl_headers = curl_slist_append(curl_headers, ((*headers)[i]).c_str());
            }
            
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

            //set data
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data->c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_request_done); 

            //set use ssl
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);


            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3); 

            res = curl_easy_perform(curl);

            if (res!=0)
                std::cout << "curl perform result: " << res << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(...)
    {
        std::cerr << "------> execute_send_message caught exception! ";
    }
    
    
}

/*
int main()
{
    DingTalk d;
    for(int i=0; i!=3; i++)
    {
        d.send_message("biaoti", "warning", 1234, "something nomarl test error message");
    } 
    while(true)
    {
        sleep(5000);
    }
    return 0;
}
*/