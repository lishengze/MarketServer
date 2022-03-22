#include "server_engine.h"

#include "grpc_comm/server.h"
#include "db_engine/db_engine.h"
#include "Log/log.h"
// #include <signum-generic.h>
#include "config/config.h"
#include <signal.h>

int volatile ServerEngine::signal_sys = SIGINT;

ServerEngine::ServerEngine()
{
    GrpcServerPtr grpc_server_sptr_ = boost::make_shared<GrpcServer>(GRPC_LISTEN_IP, this);
    DBEnginePoolPtr db_engine_sptr_ = boost::make_shared<DBEnginePool>(DATABASE_INFO, this);
    bcts::comm::CommPtr comm_server_sptr = boost::make_shared<bcts::comm::Comm>(KAFKA_IP, bcts::comm::NET_TYPE::KAFKA, 
                                                                                bcts::comm::SERIALIZE_TYPE::PROTOBUF, this);
}


bcts::comm::MetaType get_test_meta()
{
    bcts::comm::MetaType test_meta;
    std::set<string> exchange_set;
    exchange_set.emplace("FTX");
    test_meta["BTC_USDT"] = exchange_set;
    return test_meta;
}

void ServerEngine::start()
{
    try
    {
        bcts::comm::MetaType test_meta = get_test_meta();

        // comm_server_sptr->set_kline_meta(test_meta);
        comm_server_sptr->set_trade_meta(test_meta);

        comm_server_sptr->launch();
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
        // insert_kline_data(kline_data);
        LOG_INFO(kline_data.str());
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
        ReqKlineData req_kline;
        req_kline.exchange = req_trade.exchange;
        req_kline.symbol = req_trade.symbol;


        req_kline.start_time = req_trade.time;
        req_kline.end_time = req_trade.time;

        req_kline.count = 1;

        KlineData kline_data;

        db_engine_sptr_->get_nearest_kline(req_kline, kline_data);

        dst_trade_data.exchange = req_trade.exchange;
        dst_trade_data.symbol = req_trade.symbol;

        dst_trade_data.price = kline_data.px_close;
        dst_trade_data.volume = kline_data.volume;

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
