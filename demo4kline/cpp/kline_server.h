#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"

#include "redis_kline_source.h"
#include "kline_mixer.h"
#include "kline_database.h"
#include "server_endpoint.h"

class KlineServer
{
public:
    KlineServer();
    ~KlineServer();

    void start();

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);
private:

    // 从redis接入单个行情的K线
    RedisKlineSource kline_source_;

    // 聚合K线
    // 缓存短期K线，计算聚合结果
    KlineMixer kline_mixer_;

    // 数据库功能实现
    // 入数据库接口
    // 从数据库查询接口
    KlineDatabase kline_db_;

    // 服务
    // grpc模式，stream推送最新1分钟，5分钟，60分钟和日数据
    ServerEndpoint server_endpoint_;
};