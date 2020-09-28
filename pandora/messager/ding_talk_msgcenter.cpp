

#include "ding_talk_msgcenter.h"

USING_PANDORA_NAMESPACE
using namespace std;

DingTalkMsgcenter::DingTalkMsgcenter(io_service_pool& pool, const string &url, const string &app)
    : ThreadBasePool{pool}
    , asiohttp::Client(pool.get_io_service())
    , url_(url)
    , app_(app)
{
    std::string err;
    if (!asiohttp::InitSSL(&err)) {
        printf("InitSSL: %s\n\n", err.c_str());
        return;
    }


//    SetProxy("172.25.1.77", 8118);
}

void DingTalkMsgcenter::send_message(const string &title, const string &level, int error_id, const string &errorMsg, bool emgency, bool at_all){
    send_message(title.c_str(), level.c_str(), error_id, errorMsg.c_str(), at_all, emgency);
}

void DingTalkMsgcenter::send_message(const char* title, const char* level, int error_id, const char* errorMsg, bool emgency, bool at_all){
    char tpl[128]= {0};
    if (emgency) {
        sprintf(tpl, "app=%s&title=%s&level=%s&object=boss&type=see&atall=%d",
                app_.c_str(), title, level, at_all);
    } else {
        sprintf(tpl, "app=%s&title=%s&level=%s&object=boss&type=not_see&atall=%d",
                app_.c_str(), title, level, at_all);
    }

    string body = (string)tpl+"&text="+errorMsg;
    auto st1 = asiohttp::Request::Create(
            "POST",
            url_,
            body,
            requset_id_++);
    st1.request->GetHeader()->Set("Content-Type", "application/x-www-form-urlencoded");
    Do(st1.request);
    return;

}


void DingTalkMsgcenter::OnResponse(const asiohttp::Response &rsp) {

}
