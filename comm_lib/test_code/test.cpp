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
        cout << trade.str() << endl;
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

void test_consume()
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


void test_produce()
{
    TestEngine engine;
    string server_address = "127.0.0.1:9117";

    Comm comm(server_address, NET_TYPE::KAFKA, SERIALIZE_TYPE::PROTOBUF, &engine);    
    
    for (int i=0; i < 1000; ++i)
    {   
        TradeData trade_data;

        trade_data.symbol = "BTC_USDT";
        trade_data.exchange = "FTX";
        trade_data.price = 42380.5 + i;

        trade_data.time = utrade::pandora::NanoTime();
        trade_data.volume = 0.1 + i;
        trade_data.sequence_no = i;

        comm.publish_trade(trade_data);

        std::this_thread::sleep_for(std::chrono::seconds{3});
    }
}

void TestMain()
{
    test_consume();

    // test_time();

    // test_produce();
}
