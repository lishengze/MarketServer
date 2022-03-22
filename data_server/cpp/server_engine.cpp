#include "server_engine.h"

#include "grpc_comm/server.h"
#include "db_engine/db_engine.h"
#include "Log/log.h"
#include <signum-generic.h>
#include "config/config.h"


ServerEngine::ServerEngine()
{
    GrpcServerPtr grpc_server_sptr_ = boost::make_shared<GrpcServer>(GRPC_LISTEN_IP, this);
    DBEnginePoolPtr db_engine_sptr_ = boost::make_shared<DBEnginePool>(DATABASE_INFO, this);
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

        std::list<KlineData> hist_kline_data;

        db_engine_sptr_->get_kline_data_list(req_kline, hist_kline_data);

        if (hist_kline_data.size() == 1)
        {
            dst_trade_data.exchange = req_trade.exchange;
            dst_trade_data.symbol = req_trade.symbol;
            dst_trade_data.price = hist_kline_data.front().px_close;
            dst_trade_data.volume = hist_kline_data.front().volume;
            return true;
        }
        else
        {
            dst_trade_data.exchange = req_trade.exchange;
            dst_trade_data.symbol = req_trade.symbol;
            dst_trade_data.price = 0;
            dst_trade_data.volume = 0;

            return false;
        }
        

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    reutrn false;
    
}