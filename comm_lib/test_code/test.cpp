#include "base/cpp/basic.h"
#include "test.h"

#include "../cpp/comm.h"
#include "../cpp/comm_interface_define.h"

USING_COMM_NAMESPACE

class TestEngine:public bcts::comm::QuoteSourceCallbackInterface
{
    // 行情接口
    virtual void on_snap( SDepthQuote& quote) {
        // cout << quote.basic_str() << endl;
    };

    // K线接口
    virtual void on_kline( KlineData& kline) {
        // cout << kline.str() << endl;
    };

    // 交易接口
    virtual void on_trade( TradeData& trade) {
        // cout << trade.str() << endl;
    };
};

MetaType get_test_meta()
{
    MetaType test_meta;
    std::set<string> exchange_set;
    exchange_set.emplace("FTX");
    test_meta["BTC_USDT"] = exchange_set;
    return test_meta;
}

void test_code()
{
    TestEngine engine;
    string server_address = "43.154.179.47:9117";

    Comm comm(server_address, NET_TYPE::KAFKA, SERIALIZE_TYPE::PROTOBUF, &engine);

    MetaType test_meta{std::move(get_test_meta())};
    
    // comm.set_depth_meta(test_meta);

    // comm.set_kline_meta(test_meta);

    comm.set_trade_meta(test_meta);

    // comm.set_meta(test_meta, test_meta, test_meta);

    comm.launch();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void test_time()
{
    cout << utrade::pandora::NanoTime() << endl;
}

void TestMain()
{
    test_code();

    // test_time();
}
