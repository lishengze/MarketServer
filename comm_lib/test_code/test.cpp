#include "base/cpp/basic.h"
#include "test.h"

#include "../cpp/comm.h"
#include "../cpp/comm_interface_define.h"
#include "../cpp/rpc_server.h"

USING_COMM_NAMESPACE

class TestEngine:public bcts::comm::QuoteSourceCallbackInterface
{
    // 行情接口
    virtual void on_snap( SDepthQuote& quote) {
        // cout << quote.basic_str() << endl;
        COMM_LOG_INFO(quote.str());
    };

    // K线接口
    virtual void on_kline( KlineData& kline) {
        // cout << kline.str() << endl;
        COMM_LOG_INFO(kline.str());
    };

    // 交易接口
    virtual void on_trade( TradeData& trade) {
        cout << trade.str() << endl;
        COMM_LOG_INFO(trade.str());
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

void test_grpc_server()
{
    cout << "test_grpc_server " << endl;
    bcts::comm::GrpcServer server("0.0.0.0:5008");

    server.start();   
}

void test_consume()
{
    TestEngine engine;
    string server_address = "127.0.0.1:9117";

    Comm comm(server_address, NET_TYPE::KAFKA, SERIALIZE_TYPE::PROTOBUF, &engine);

    MetaType test_meta{std::move(get_test_meta())};
    
    // comm.set_depth_meta(test_meta);

    comm.set_kline_meta(test_meta);

    // comm.set_trade_meta(test_meta);

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
    
    // for (int i=0; i < 1000; ++i)
    // {   
    //     TradeData trade_data;

    //     trade_data.symbol = "BTC_USDT";
    //     trade_data.exchange = "FTX";
    //     trade_data.price = 42380.5 + i;

    //     trade_data.time = utrade::pandora::NanoTime();
    //     trade_data.volume = 0.1 + i;
    //     trade_data.sequence_no = i;

    //     comm.publish_trade(trade_data);

    //     std::this_thread::sleep_for(std::chrono::seconds{3});
    // }

    for (int i=0; i < 1000; ++i)
    {   
        KlineData kline_data;

        kline_data.symbol = "BTC_USDT";
        kline_data.exchange = "FTX";
        kline_data.px_open = 42380.5 + i;
        kline_data.px_high = 42380.5 + i + 150;
        kline_data.px_low = 42380.5 + i - 50;
        kline_data.px_close = 42380.5 + i + 100;
        

        kline_data.index = utrade::pandora::NanoTime();
        kline_data.volume = 0.1 + i;

        kline_data.sequence_no = i;

        comm.publish_kline(kline_data);

        std::this_thread::sleep_for(std::chrono::seconds{5});
    }

}

void TestMain()
{
    test_grpc_server();

    // test_consume();

    // test_time();

    // test_produce();
}
