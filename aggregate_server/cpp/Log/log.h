#pragma once

#include "base/cpp/basic.h"
#include "base/cpp/base_data_stuct.h"

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/base_log.h"


class QuoteLog: public BaseLog 
{
public:
    QuoteLog();

    virtual ~QuoteLog();

    using BaseLog::record_input_info;
    using BaseLog::record_output_info;

    virtual void log_info_(const string& info);

    void record_input_info(const string& info, const SDepthQuote& quote);

    void record_input_info(const string& info, const TradeData& trade);

    void record_input_info(const string& info, const KlineData& kline);

    void record_output_info(const string& info, const SDepthQuote& quote);

    void record_output_info(const string& info, const TradeData& trade);

    void record_output_info(const string& info, const KlineData& kline);
};

#define LOG utrade::pandora::ThreadSafeSingleton<QuoteLog>::DoubleCheckInstance()

#define LOG_TRACE(info) LOG->log_trace_(LOG_HEADER + info)
#define LOG_DEBUG(info) LOG->log_debug_(LOG_HEADER + info)
#define LOG_INFO(info) LOG->log_info_(LOG_HEADER + info)
#define LOG_WARN(info) LOG->log_warn_(LOG_HEADER + info)
#define LOG_ERROR(info) LOG->log_error_(LOG_HEADER + info)
#define LOG_FATAL(info) LOG->log_fatal_(LOG_HEADER + info)

#define LOG_CLIENT_REQUEST(info) LOG->log_client_request(LOG_HEADER + info)
#define LOG_CLIENT_RESPONSE(info) LOG->log_client_response(LOG_HEADER + info)
#define LOG_SOURCE_INPUT(info) LOG->log_source_input(LOG_HEADER + info)
