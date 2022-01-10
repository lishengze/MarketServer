#pragma once

#include "base/cpp/basic.h"

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/base_log.h"

class RiskLog:public BaseLog 
{
public:

    virtual ~RiskLog();

    virtual void log_info_(const string& info)
    {
        LOG4CPLUS_INFO(*common_logger_.get(), info);

        cout << info << endl;        
    }    

    RiskLog();
};

#define LOG utrade::pandora::ThreadSafeSingleton<RiskLog>::DoubleCheckInstance()

#define LOG_TRACE(info) LOG->log_trace_(LOG_HEADER + info)
#define LOG_DEBUG(info) LOG->log_debug_(LOG_HEADER + info)
#define LOG_INFO(info) LOG->log_info_(LOG_HEADER + info)
#define LOG_WARN(info) LOG->log_warn_(LOG_HEADER + info)
#define LOG_ERROR(info) LOG->log_error_(LOG_HEADER + info)
#define LOG_FATAL(info) LOG->log_fatal_(LOG_HEADER + info)

#define LOG_CLIENT_REQUEST(info) LOG->log_client_request(LOG_HEADER + info)
#define LOG_CLIENT_RESPONSE(info) LOG->log_client_response(LOG_HEADER + info)
#define LOG_SOURCE_INPUT(info) LOG->log_source_input(LOG_HEADER + info)