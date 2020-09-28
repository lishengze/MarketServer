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

#include <asio_httpclient/src/asio_http.h>
#include <pandora/util/io_service_pool.h>
#include <pandora/util/thread_basepool.h>
#include "../pandora_declare.h"

PANDORA_NAMESPACE_START


class DingTalkMsgcenter : public asiohttp::Client,  public ThreadBasePool
{
public:
    DingTalkMsgcenter(io_service_pool& pool, const string &url, const string &app);

    // send message to ding talk
    void send_message(const string &title, const string &level, int error_id, const string &errorMsg, bool emgency=false, bool at_all=false);
    void send_message(const char* title, const char* level, int error_id, const char* errorMsg, bool emgency=false, bool at_all=false);

private:
    virtual void OnResponse(const asiohttp::Response &rsp) override ;

    int requset_id_{0};
    string url_;
    string app_;
};

DECLARE_PTR(DingTalkMsgcenter);

PANDORA_NAMESPACE_END