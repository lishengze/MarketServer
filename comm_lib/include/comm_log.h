#pragma once

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/base_log.h"
#include "base/cpp/basic.h"

class CommLog: public BaseLog 
{
public:
    virtual void log_info_(const string& info)
    {
        LOG4CPLUS_INFO(*common_logger_.get(), info);

        cout << info << endl;
    }
};


#define COMM_LOG utrade::pandora::ThreadSafeSingleton<CommLog>::DoubleCheckInstance()

#define COMM_LOG_TRACE(info) COMM_LOG->log_trace_(LOG_HEADER + info)
#define COMM_LOG_DEBUG(info) COMM_LOG->log_debug_(LOG_HEADER + info)
#define COMM_LOG_INFO(info) COMM_LOG->log_info_(LOG_HEADER + info)
#define COMM_LOG_WARN(info) COMM_LOG->log_warn_(LOG_HEADER + info)
#define COMM_LOG_ERROR(info) COMM_LOG->log_error_(LOG_HEADER + info)
#define COMM_LOG_FATAL(info) COMM_LOG->log_fatal_(LOG_HEADER + info)

