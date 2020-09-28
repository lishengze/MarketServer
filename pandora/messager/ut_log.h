/*
 * @Author: daniel.bian
 * @Date: 2018-10-22
 * @Last Modified by: daniel.bian
 * @Last Modified Date: 2018-11-14
 * @des: this file used for helping to log
 */
#pragma once

#include "../pandora_declare.h"
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/loglevel.h>
#include <boost/shared_ptr.hpp>

// universal trading log recorder
#define UT_LOG_FATAL(UTLogger, content) LOG4CPLUS_FATAL(UTLogger->getLogger(), content)
#define UT_LOG_ERROR(UTLogger, content) LOG4CPLUS_ERROR(UTLogger->getLogger(), content)
#define UT_LOG_WARNING(UTLogger, content) LOG4CPLUS_WARN(UTLogger->getLogger(), content) 
#define UT_LOG_INFO(UTLogger, content) LOG4CPLUS_INFO(UTLogger->getLogger(), content)
#define UT_LOG_DEBUG(UTLogger, content) LOG4CPLUS_DEBUG(UTLogger->getLogger(), content)
#define UT_LOG_TRACE(UTLogger, content) LOG4CPLUS_TRACE(UTLogger->getLogger(), content)

#define UT_LOG_FATAL_FMT(UTLogger, content, ...) LOG4CPLUS_FATAL_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)
#define UT_LOG_ERROR_FMT(UTLogger, content, ...) LOG4CPLUS_ERROR_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)
#define UT_LOG_WARNING_FMT(UTLogger, content, ...) LOG4CPLUS_WARN_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)
#define UT_LOG_INFO_FMT(UTLogger, content, ...) LOG4CPLUS_INFO_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)
#define UT_LOG_DEBUG_FMT(UTLogger, content, ...) LOG4CPLUS_DEBUG_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)
#define UT_LOG_TRACE_FMT(UTLogger, content, ...) LOG4CPLUS_TRACE_FMT(UTLogger->getLogger(), content, ##__VA_ARGS__)

PANDORA_NAMESPACE_START

FORWARD_DECLARE_PTR(UTLog);

class UTLog
{
protected:
    // the actual log writer
    log4cplus::Logger logger;

protected:
    UTLog() {};
    UTLog(const string& log_unique_name, const string& config_dir, const string& config_name = "");

public:

    void set_log_level(const char& level);
    inline log4cplus::Logger& getLogger(){
        return logger;
    }
    inline void fatal(const char* content){
        LOG4CPLUS_FATAL(logger, content);
    }
    inline void error(const char* content){
        LOG4CPLUS_ERROR(logger, content);
    }
    inline void info(const char* content){
        LOG4CPLUS_INFO(logger, content);
    }
    inline void debug(const char* content){
        LOG4CPLUS_DEBUG(logger, content);
    }

    static string getConfigFolder();

    // attention: return true if really configured.
    static bool doConfigure(string configureName);

    static UTLogPtr getLogger(const string& log_unique_name, const string& config_dir=string(UTRADE_INSTALL_PATH), const string& config_name = "");

    static UTLogPtr getStrategyLogger(const string& log_unique_name, const string& log_file_name);

    static UTLogPtr getStrategySysLogger(const string& log_unique_name, const string& log_file_name, const string& config_dir="");

    static UTLogPtr getStrategyPackageLogger(const string& log_unique_name, const string& log_file_name, const string& config_dir="");
};

class UTLogStrategy: public UTLog
{
public:
    UTLogStrategy(const string& log_unique_name, const string& log_file_name);
};

class UTLogStrategySys : public UTLog
{
public:
    UTLogStrategySys(const string& log_unique_name, const string& log_file_name, const string& config_dir);
};

class UTLogStrategyPackage : public UTLog
{
public:
    UTLogStrategyPackage(const string& log_unique_name, const string& log_file_name, const string& config_dir);
};

PANDORA_NAMESPACE_END