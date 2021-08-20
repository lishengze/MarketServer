#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <log4cplus/log4cplus.h>

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/util/time_util.h"
#include "pandora/log/base_log.h"


#include "hub_interface.h"
#include "hub_struct.h"
#include "../data_structure/base_data.h"
#include "../front_server_declare.h"


using std::string;
using std::cout;
using std::endl;

class FrontServerLog:public BaseLog 
{
    public:

        using BaseLog::record_input_info;
        using BaseLog::record_output_info;

        FrontServerLog();
        
        virtual ~FrontServerLog();

        virtual void record_input_info(const string& channel, const SDepthData& quote);

        virtual void record_input_info(const string& channel, const Trade& trade);

        virtual void record_input_info(const string& channel, const vector<KlineData>& klines);        

        virtual void record_output_info(const string& channel, const SDepthData& quote);

        virtual void record_output_info(const string& channel, const Trade& trade);

        virtual void record_output_info(const string& channel, const vector<KlineData>& klines);

        virtual void record_output_info(const string& channel, const std::vector<KlineDataPtr>& klines);

        virtual void record_client_info(const string& client_id, const string& info);

        virtual void log_debug(const string& info);
};

#define LOG utrade::pandora::ThreadSafeSingleton<FrontServerLog>::DoubleCheckInstance()

#define LOG_TRACE(info) LOG->log_trace_(LOG_HEADER + info)
#define LOG_DEBUG(info) LOG->log_debug_(LOG_HEADER + info)
#define LOG_INFO(info) LOG->log_info_(LOG_HEADER + info)
#define LOG_WARN(info) LOG->log_warn_(LOG_HEADER + info)
#define LOG_ERROR(info) LOG->log_error_(LOG_HEADER + info)
#define LOG_FATAL(info) LOG->log_fatal_(LOG_HEADER + info)

#define LOG_CLIENT_REQUEST(info) LOG->log_client_request(LOG_HEADER + info)
#define LOG_CLIENT_RESPONSE(info) LOG->log_client_response(LOG_HEADER + info)
#define LOG_SOURCE_INPUT(info) LOG->log_source_input(LOG_HEADER + info)