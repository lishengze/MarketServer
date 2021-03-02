#include "nacos_client.h"
#include "tinyformat.h"

void NacosListener::init(ConfigService* service, string group, string dataid, NacosClient* parent) 
{
    group_ = group;
    dataid_ = dataid;
    parent_ = parent;

    // init config
    NacosString ss = "";
    while( true ) {
        try {
            ss = service->getConfig(dataid_, group_, 1000);
            break;
        }
        catch (NacosException &e) {
            tfm::printfln("request %s-%s errcode: %d Reason: %s", group_, dataid_, e.errorcode(), e.what());
        }
    }
    receiveConfigInfo(ss);

    // register for update
    service->addListener(dataid_, group_, this);
}

void NacosListener::receiveConfigInfo(const NacosString &configInfo) 
{
    parent_->config_changed(group_, dataid_, configInfo);
}

void NacosClient::start(const string& addr, const string& ns) 
{
    thread_run_ = true;

    Properties props;
    props[PropertyKeyConst::SERVER_ADDR] = addr;
    props[PropertyKeyConst::NAMESPACE] = ns;
    NacosServiceFactory *factory = new NacosServiceFactory(props);
    //ResourceGuard <NacosServiceFactory> _guardFactory(factory);
    service_ = factory->CreateConfigService();
    //ResourceGuard <ConfigService> _serviceFactory(service_);

    _run_thread_ = new std::thread(&NacosClient::_run, this);
}

void NacosClient::add_listener(const string& group, const string& dataid, NacosListener& listener)
{
    listener.init(service_, group, dataid, this);
}