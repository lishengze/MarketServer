#include "nacos_client.h"
#include "base/cpp/quote.h"
#include "stream_engine_define.h"
#include "stream_engine_config.h"

NacosListener::NacosListener(ConfigService* server, string group, string dataid, INacosCallback* callback) 
{
    group_ = group;
    dataid_ = dataid;
    server_ = server;
    callback_ = callback;

    NacosString ss = "";
    try {
        ss = server_->getConfig(dataid_, group_, 1000);
    }
    catch (NacosException &e) {
        cout <<
            "Request failed with curl code:" << e.errorcode() << endl <<
            "Reason:" << e.what() << endl;
        return;
    }
    on_get_config(ss);
}

void NacosListener::on_get_config(const NacosString &configInfo) const
{
    if( group_ == "quotation" && dataid_ == "symbols" ) {
        callback_->on_config_channged(configInfo);
    }
}

void NacosListener::receiveConfigInfo(const NacosString &configInfo) {
    on_get_config(configInfo);
}


void NacosClient::start(const string& addr, INacosCallback* callback) 
{
    _log_and_print("connect nacos addr %s.", addr.c_str());
    _run_thread_ = new std::thread(&NacosClient::_run, this, addr, callback);
}
