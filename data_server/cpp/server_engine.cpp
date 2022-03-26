#include "server_engine.h"

#include "Log/log.h"
#include "config/config.h"
#include <signal.h>

int volatile ServerEngine::signal_sys = SIGINT;

USING_COMM_NAMESPACE

ServerEngine::ServerEngine()
{
    grpc_server_sptr_ = boost::make_shared<GrpcServer>(GRPC_LISTEN_IP, this);

    db_engine_sptr_ = boost::make_shared<DBEnginePool>(DATABASE_INFO, this);

    comm_server_sptr = boost::make_shared<Comm>(KAFKA_IP, NET_TYPE::KAFKA, SERIALIZE_TYPE::PROTOBUF, this);
}

MetaType get_test_kline_meta()
{
    MetaType test_meta;
    std::set<string> exchange_set;
    exchange_set.emplace("FTX");
    test_meta["BTC_USDT"] = exchange_set;
    return test_meta;
}


void ServerEngine::start()
{
    try
    {
        LOG_INFO("Start");

        grpc_server_sptr_->start();

        bcts::comm::MetaType test_meta = get_test_kline_meta();

        comm_server_sptr->set_kline_meta(test_meta);

        comm_server_sptr->launch();

        LOG_INFO("Start End!");
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void ServerEngine::on_kline(KlineData& kline_data)
{
    try
    {
        LOG_INFO(kline_data.str());
        insert_kline_data(kline_data);       
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void ServerEngine::on_trade( TradeData& trade)
{
    try
    {
        LOG_INFO(trade.meta_str());
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void ServerEngine::insert_kline_data(const KlineData& kline_data)
{
    try
    {
        db_engine_sptr_->insert_kline_data(kline_data);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

bool ServerEngine::get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data)
{
    try
    {
        LOG_INFO("ReqTradeData: " + req_trade.str());

        ReqKlineData req_kline;
        req_kline.exchange = req_trade.exchange;
        req_kline.symbol = req_trade.symbol;


        req_kline.start_time = req_trade.time;
        req_kline.end_time = req_trade.time;

        req_kline.count = 1;

        KlineData kline_data;

        db_engine_sptr_->get_nearest_kline(req_kline, kline_data);

        dst_trade_data.time = kline_data.index;
        dst_trade_data.exchange = req_trade.exchange;
        dst_trade_data.symbol = req_trade.symbol;

        dst_trade_data.price = kline_data.px_close;
        dst_trade_data.volume = kline_data.volume;

        LOG_INFO("DST TRADE: " + dst_trade_data.str());
        LOG_INFO("KLINE: " + kline_data.str());

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    return false;    
}

string get_signum_str(int signum)
{
    string result ="unknown signum: " + std::to_string(signum);

    switch (signum)
    {
        case SIGTERM:
            result = "SIGTERM";
            break;
        case SIGINT:
            result = "SIGINT";
            break;

        case SIGHUP:
            result = "SIGHUP";
            break;

        case SIGQUIT:
            result = "SIGQUIT";
            break;

        case SIGKILL:
            result = "SIGKILL";
            break;
        case SIGABRT:
            result = "SIGABRT";
            break;
        case SIGSEGV:
            result = "SIGSEGV";
            break;                                            
        default:
            break;
    }
    return result;
}

void ServerEngine::signal_handler(int signum)
{
//    UT_LOG_INFO(WORMHOLE_LOGGER, "[ServerEngine], signal_handler " << signum);
    // printf("\n[ServerEngine], signal_handler:%d \n", signum);

    LOG_ERROR("[ServerEngine], signal_handler: " + get_signum_str(signum));
    signal_sys = signum;
    // 释放资源
    // SERVER_EENGINE->release();
    // 退出
    exit(-1);
}
