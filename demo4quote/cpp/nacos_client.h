#pragma once

#include "stream_engine_define.h"
#include "factory/NacosServiceFactory.h"
#include "ResourceGuard.h"
#include "listen/Listener.h"
#include "PropertyKeyConst.h"
#include "DebugAssertion.h"
#include "Debug.h"

#include "base/cpp/quote.h"
#include "stream_engine_config.h"

using namespace std;
using namespace nacos;

// 从nacos接入
// - 行情涉及的币对和交易所
// - dataID = symbols, groupID = quotation


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

struct SNacosConfigFee
{
    int fee_type;
    float fee_maker;
    float fee_taker;
};

struct SNacosConfig
{
    int precise;
    int depth;
    float frequency;
    int mix_depth;
    float mix_frequecy;
    map<TExchange, SNacosConfigFee> exchanges;    
};

class INacosCallback {
public:
    virtual void on_symbol_channged(const std::unordered_map<TSymbol, SNacosConfig>& symbols) = 0;
};

class NacosListener : public Listener {
private:
    string group_;
    string dataid_;
    ConfigService* server_;
    INacosCallback* callback_;
public:
    NacosListener(ConfigService* server, string group, string dataid, INacosCallback* callback) {
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
        receiveConfigInfo(ss);
    }

    void receiveConfigInfo(const NacosString &configInfo) {
        if( group_ == "quotation" && dataid_ == "symbols" ) {
            _log_and_print("quotation:symbols changed. %s", configInfo.c_str());

            njson js = njson::parse(configInfo);
            std::unordered_map<TSymbol, SNacosConfig> symbols;
            for (auto iter = js.begin() ; iter != js.end() ; ++iter )
            {
                const TSymbol& symbol = iter.key();
                const njson& symbol_cfgs = iter.value();
                SNacosConfig cfg;
                cfg.precise = symbol_cfgs["precise"].get<int>();
                cfg.depth = symbol_cfgs["depth"].get<int>();
                cfg.frequency = symbol_cfgs["frequency"].get<float>();
                cfg.mix_depth = symbol_cfgs["mix_depth"].get<int>();
                cfg.mix_frequecy = symbol_cfgs["mix_frequency"].get<float>();
                for( auto iter2 = symbol_cfgs["exchanges"].begin() ; iter2 != symbol_cfgs["exchanges"].end() ; ++iter )
                {
                    const TExchange& exchange = iter2.key();
                    const njson& exchange_cfgs = iter2.value();
                    SNacosConfigFee exchange_cfg;
                    exchange_cfg.fee_type = exchange_cfgs["fee_type"].get<int>();
                    exchange_cfg.fee_maker = exchange_cfgs["fee_maker"].get<float>();
                    exchange_cfg.fee_taker = exchange_cfgs["fee_taker"].get<float>();
                    cfg.exchanges[exchange] = exchange_cfg;
                } 
                symbols[symbol] = cfg;
            }
            callback_->on_symbol_channged(symbols);
        }
    }
};

class NacosClient
{
public:
    NacosClient(){}
    ~NacosClient(){}

    void start(const string& addr, INacosCallback* callback) 
    {
        _run_thread_ = new std::thread(&NacosClient::_run, this, addr, callback);
        cout << "nacos started." << endl;
    }

private:
    std::thread* _run_thread_ = nullptr;
    void _run(const string& addr, INacosCallback* callback)
    {
        Properties props;
        props[PropertyKeyConst::SERVER_ADDR] = addr; // "36.255.220.139:8848"
        props[PropertyKeyConst::NAMESPACE] = "bcts";
        NacosServiceFactory *factory = new NacosServiceFactory(props);
        ResourceGuard <NacosServiceFactory> _guardFactory(factory);
        ConfigService *n = factory->CreateConfigService();
        ResourceGuard <ConfigService> _serviceFactory(n);

        try {
            NacosListener *listener1 = new NacosListener(n, "quotation", "symbols", callback);
            n->addListener("symbols", "quotation", listener1);
        }
        catch (NacosException &e) {
            cout <<
                "Request failed with curl code:" << e.errorcode() << endl <<
                "Reason:" << e.what() << endl;
            return;
        }
    }
};
