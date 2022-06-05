#include "test.h"
#include "Log/log.h"
#include "grpc_client.h"

#include "pandora/util/time_util.h"

void test_grpc_client()
{
    GrpcClient client("0.0.0.0:5008");

    for (int i = 0; i < 1; ++i)
    {
        ReqTradeInfoLocal req_trade_info;
        req_trade_info.exchange = "FTX";
        req_trade_info.symbol = "BTC_USDT";
        req_trade_info.time = 1648210737040520318;

        client.request_trade_data(req_trade_info);   

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
 
}

void test_otc() { 
    OTCClient client("0.0.0.0:8111");
    client.OTC();
}

void TestMain()
{
    LOG_INFO("TestMain");
    
    // test_grpc_client();

    test_otc();
}