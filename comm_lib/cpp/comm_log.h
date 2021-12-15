#pragma once

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/base_log.h"


#define COMM_LOG utrade::pandora::ThreadSafeSingleton<BaseLog>::DoubleCheckInstance()

#define COMM_LOG_TRACE(info) COMM_LOG->log_trace_(LOG_HEADER + info)
#define COMM_LOG_DEBUG(info) COMM_LOG->log_debug_(LOG_HEADER + info)
#define COMM_LOG_INFO(info) COMM_LOG->log_info_(LOG_HEADER + info)
#define COMM_LOG_WARN(info) COMM_LOG->log_warn_(LOG_HEADER + info)
#define COMM_LOG_ERROR(info) COMM_LOG->log_error_(LOG_HEADER + info)
#define COMM_LOG_FATAL(info) COMM_LOG->log_fatal_(LOG_HEADER + info)

