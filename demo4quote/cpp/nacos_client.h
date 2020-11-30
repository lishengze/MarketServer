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
// field : SymbolId,MinChangePrice
// desc  : "MinChangePrice":0.1
#define CFG_SYMBOL_PRECISE "CFG_SYMBOL_PRECISE"
// group : parameter
// dataid: hedging
// field : PlatformId,Instrument,FeeKind,TakerFee,MakerFee
// desc  : "FeeKind":1, 取值1或2，1表示百比分，2表示绝对值。默认为1
// desc  : "TakerFee":1,"MakerFee":2
#define CFG_SYMBOL_FEE "CFG_SYMBOL_FEE"

// 0.1 -> 1
// 0.01 -> 2
inline int to_precise(float v) {
    int count = 0;
    float tmp = v;
    while( tmp < 1 ){        
        tmp *= 10;
        count += 1;
    }
    return count;
}

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

            std::unordered_map<string, int> changed;
            for ( const auto& v : js ) {
                const string& symbol = v["SymbolId"].get<string>();
                int precise = to_precise(v["MinChangePrice"].get<float>());
                if( last_precise_[symbol] != precise ) {
                    last_precise_[symbol] = precise;
                    changed[symbol] = precise;
                }
            }
            cout << "set_configuration_precise begin" << endl;
            CONFIG->set_configuration_precise(changed);
            cout << "set_configuration_precise end" << endl;
        } else if( group_ == "parameter" && dataid_ == "hedging" ) {
            njson js = njson::parse(configInfo);

            std::unordered_map<string, std::unordered_map<string, SymbolFee>> changed;
            for ( const auto& v : js ) {
                const string& exchange = v["PlatformId"].get<string>();
                const string& symbol = v["Instrument"].get<string>();
                SymbolFee fee;
                fee.fee_type = v["FeeKind"].get<int>();
                fee.taker_fee = v["TakerFee"].get<double>();
                fee.maker_fee = v["MakerFee"].get<double>();
                if( memcmp(&last_fee_[exchange][symbol], &fee, sizeof(fee)) != 0 ) {
                    last_fee_[exchange][symbol] = fee;
                    changed[exchange][symbol] = fee;
                }
            }
            CONFIG->set_configuration_fee(changed);
        }
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

        try {
            NacosListener *listener1 = new NacosListener(n, "parameter", "symbol");
            n->addListener("symbol", "parameter", listener1);
            NacosListener *listener2 = new NacosListener(n, "parameter", "hedging");
            n->addListener("hedging", "parameter", listener2);
        }
        catch (NacosException &e) {
            cout <<
                "Request failed with curl code:" << e.errorcode() << endl <<
                "Reason:" << e.what() << endl;
            return;
        }    
    }
};