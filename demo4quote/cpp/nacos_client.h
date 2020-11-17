#pragma once

#include "stream_engine_define.h"
#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"

#include "stream_engine_config.h"

using namespace std;
using namespace nacos;

// 直接从数据库接入
#define CFG_INCLUDE_SYMBOLS "CFG_INCLUDE_SYMBOLS"
// 直接从数据库接入
#define CFG_INCLUDE_EXCHANGES "CFG_INCLUDE_EXCHANGES"
// 暂无
#define CFG_FORBIDDEN_EXCHANGES "CFG_FORBIDDEN_EXCHANGES"
// group : parameter
// dataid: symbol
// field : MinChangePrice
// desc  : "MinChangePrice":0.1
#define CFG_SYMBOL_PRECISE "CFG_SYMBOL_PRECISE"
// group : parameter
// dataid: symbol
// field : FeeKind,TakerFee,MakerFee
// desc  : "FeeKind":1, 取值1或2，1表示百比分，2表示绝对值。默认为1
// desc  : "TakerFee":1,"MakerFee":2
#define CFG_SYMBOL_FEE "CFG_SYMBOL_FEE"


class NacosListener : public Listener {
private:
    string group_;
    string dataid_;
    ConfigService* server_;
    
    std::unordered_map<string, int> last_precise_;
    std::unordered_map<string, std::unordered_map<string, SymbolFee>> last_fee_;
public:
    NacosListener(ConfigService* server, string group, string dataid) {
        group_ = group;
        dataid_ = dataid;
        server_ = server;

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
        receiveConfigInfo(ss);
    }

    void receiveConfigInfo(const NacosString &configInfo) {
        if( group_ == "parameter" && dataid_ == "symbol" ) {
            njson js = njson::parse(configInfo);

            std::unordered_map<string, int> precise;
            precise["BTC_USDT"] = 1;
            precise["ETH_USDT"] = 2;
            precise["ETH_BTC"] = 6;
            std::unordered_map<string, std::unordered_map<string, SymbolFee>> fee;
            CONFIG->set_configuration_precise(precise);
            CONFIG->set_configuration_fee(fee);
        }
        
        //cout << "===================================" << endl;
        //cout << "Watcher" << num << endl;
        //cout << "Watched Key UPDATED:" << configInfo << endl;
        //cout << "===================================" << endl;
    }
};

class NacosClient
{
public:
    NacosClient(){}
    ~NacosClient(){}

    void start() 
    {
        Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = CONFIG->nacos_addr_; // "36.255.220.139:8848"
        props[PropertyKeyConst::NAMESPACE] = "bcts";
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        //NacosListener *listener1 = new NacosListener(n, "parameter", "currency");//You don't need to free it, since it will be deleted by the function removeListener
        //n->addListener("currency", "parameter", listener1);//All changes on the key dqid will be received by MyListener
    }
private:

};
