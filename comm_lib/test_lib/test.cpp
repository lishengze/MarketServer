#include "base/cpp/basic.h"
#include "test.h"

#include "../cpp/comm.h"
#include "../cpp/interface_define.h"


class TestEngine:public bcts::comm::QuoteSourceCallbackInterface
{
    // 行情接口
    virtual void on_snap( SDepthQuote& quote) {
        cout << quote.basic_str() << endl;
    };

    // K线接口
    virtual void on_kline( KlineData& kline) {
        cout << kline.str() << endl;
    };

    // 交易接口
    virtual void on_trade( TradeData& trade) {
        cout << trade.str() << endl;
    };
};

void test_lib()
{

}

void test_code()
{

}

void TestMain()
{
    test_code();
}
