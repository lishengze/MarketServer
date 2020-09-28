#include "ut_log.h"

#include <log4cplus/initializer.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include "../util/path_util.h"

// 子配置目录
#define LOG_CONFIGURATION_SUB_FOLDER "/etc/log4cplus/"
// 平台配置文件
#define LOG_CONFIGURATION_BASIC_FILENAME "default.properties"
// 平台配置文件路经
#define LOG_CONFIGURATION_BASIC_FILE LOG_CONFIGURATION_SUB_FOLDER LOG_CONFIGURATION_BASIC_FILENAME

// 策略日志文件存储目录
#define STRATEGY_SYS_LOG_FOLDER UTRADE_DATA_FOLDER "log/singularity/strategy/"
// 策略系统配置文件
#define LOG_CONFIGURATION_STRATEGY_SYS_PATTERN_FILENAME "strategy.pattern"
// 策略配置文件路经
#define LOG_CONFIGURATION_STRATEGY_SYS_PATTERN_FILE LOG_CONFIGURATION_SUB_FOLDER LOG_CONFIGURATION_STRATEGY_SYS_PATTERN_FILENAME

// 策略数据包文件存储目录
#define STRATEGY_PACKAGE_LOG_FOLDER UTRADE_DATA_FOLDER "log//singularity/package/"
// 策略系统配置文件
#define LOG_CONFIGURATION_PACKAGE_PATTERN_FILENAME "package.pattern"
// 策略配置文件路经
#define LOG_CONFIGURATION_PACKAGE_PATTERN_FILE LOG_CONFIGURATION_SUB_FOLDER LOG_CONFIGURATION_PACKAGE_PATTERN_FILENAME


// 策略用户文件路经
#define STRATEGY_LOG_FOLDER "./log/"

// 文件滚动配置
#define STRATEGY_LOG_MAX_FILE_SIZE 1000 * 1024 * 1024
#define STRATEGY_LOG_MAX_BACKUP_INDEX 10
#define STRATEGY_PACKAGE_LOG_MAX_FILE_SIZE 1000*1024*1024
#define STRATEGY_PACKAGE_LOG_MAX_BACKUP_INDEX 10

USING_PANDORA_NAMESPACE
using namespace log4cplus;

static bool configured = false;

bool UTLog::doConfigure(string configureName)
{
    if (!configured)
    {
        log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(configureName));
        configured = true;
        return true;
    }
    else
    {
        return false;
    }
}

UTLogPtr UTLog::getLogger(const string& log_unique_name, const string& config_dir/* ="" */ , const string& config_name)
{
    return UTLogPtr(new UTLog(log_unique_name, config_dir, config_name));
}

void UTLog::set_log_level(const char& level)
{
    switch (level)
    {
        case '0':
            logger.setLogLevel(TRACE_LOG_LEVEL);
            break;
        case '1':
            logger.setLogLevel(DEBUG_LOG_LEVEL);
            break;
        case '2':
            logger.setLogLevel(INFO_LOG_LEVEL);
            break;
        case '3':
            logger.setLogLevel(WARN_LOG_LEVEL);
            break;
        case '4':
            logger.setLogLevel(ERROR_LOG_LEVEL);
            break;
        case '5':
            logger.setLogLevel(FATAL_LOG_LEVEL);
            break;
        default:
            logger.setLogLevel(DEBUG_LOG_LEVEL);
            break;
    }
}

UTLogPtr UTLog::getStrategyLogger(const string& log_unique_name, const string& log_file_name)
{
    return UTLogPtr(new UTLogStrategy(log_unique_name, log_file_name));
}

UTLogPtr UTLog::getStrategySysLogger(const string& log_unique_name, const string& log_file_name, const string& config_dir/* ="" */)
{
    return UTLogPtr(new UTLogStrategySys(log_unique_name, log_file_name, config_dir));
}

UTLogPtr UTLog::getStrategyPackageLogger(const string& log_unique_name, const string& log_file_name, const string& config_dir/* ="" */)
{
    return UTLogPtr(new UTLogStrategyPackage(log_unique_name, log_file_name, config_dir));
}

//wang.hy 20200119 增加config_name, 可以指定使用的log4plus的具体配置
UTLog::UTLog(const string& log_unique_name, const string& config_dir, const string& config_name)
{
    if (config_dir.empty())
        doConfigure(get_module_path() + LOG_CONFIGURATION_BASIC_FILE);
    else
    {
        if(config_name.empty())
            doConfigure(config_dir + LOG_CONFIGURATION_SUB_FOLDER + LOG_CONFIGURATION_BASIC_FILENAME);
        else
            doConfigure(config_dir + LOG_CONFIGURATION_SUB_FOLDER + config_name);
    }
    
    // std::cout << "Log Folder: " << LOG_CONFIGURATION_BASIC_FILE << std::endl;
    logger = log4cplus::Logger::getInstance(log_unique_name);
}

string UTLog::getConfigFolder()
{
    return get_module_path();
}

UTLogStrategy::UTLogStrategy(const string& log_unique_name, const string& log_file_name)
{
    if (configured)
        throw std::runtime_error("UTLogStrategy error: duplicate configuration!");
    // save pattern
    string pattern("%D{%y-%m-%d %H:%M:%S.%q} %-5p %c{2} %%%x%% - %m %n");

    // 检查策略的日志文件夹是否存在
    CheckPath(STRATEGY_LOG_FOLDER);
    std::string log_path{STRATEGY_LOG_FOLDER + log_file_name};
    if (log_file_name.find('.') == string::npos)
        log_path += ".log";

    // file appender
    SharedAppenderPtr fileAppender(new RollingFileAppender(log_path, STRATEGY_LOG_MAX_FILE_SIZE, STRATEGY_LOG_MAX_BACKUP_INDEX));
    fileAppender->setLayout(std::unique_ptr<Layout>(new PatternLayout(pattern)));

    // final
    logger = Logger::getInstance(log_unique_name);
    logger.addAppender(fileAppender);

    logger.setLogLevel(DEBUG_LOG_LEVEL);
    configured = true;
}

UTLogStrategySys::UTLogStrategySys(const string& log_unique_name, const string& log_file_name, const string& config_dir)
{
    string config_path{};
    if (config_dir.empty())
        config_path = get_module_path();
    else
        config_path = config_dir;
    string pattern;
    std::cout << "UTLogStrategySys Config Path: " <<  config_path + LOG_CONFIGURATION_STRATEGY_SYS_PATTERN_FILE << std::endl;
    std::ifstream fin(config_path + LOG_CONFIGURATION_STRATEGY_SYS_PATTERN_FILE);
    std::getline(fin, pattern);

    CheckPath(STRATEGY_SYS_LOG_FOLDER);
    string log_path{STRATEGY_SYS_LOG_FOLDER + log_file_name};
    if (log_file_name.find('.') == string::npos)
        log_path += ".log";

    // file appender
    SharedAppenderPtr fileAppender(new RollingFileAppender(log_path, STRATEGY_LOG_MAX_FILE_SIZE, STRATEGY_LOG_MAX_BACKUP_INDEX));
    fileAppender->setLayout(std::unique_ptr<Layout>(new PatternLayout(pattern)));
    // console appender
    SharedAppenderPtr consoleAppender(new ConsoleAppender());
    consoleAppender->setLayout(std::unique_ptr<Layout>(new PatternLayout(pattern)));

    // final
    logger = Logger::getInstance(log_unique_name);
    // append appender
    logger.addAppender(consoleAppender);
    logger.addAppender(fileAppender);

    logger.setLogLevel(DEBUG_LOG_LEVEL);
}

UTLogStrategyPackage::UTLogStrategyPackage(const string& log_unique_name, const string& log_file_name, const string& config_dir)
{
    string config_path{};
    if (config_dir.empty())
        config_path = get_module_path();
    else
        config_path = config_dir;
    string pattern;
    std::cout << "UTLogStrategyPackage Config Path: " <<  config_path + LOG_CONFIGURATION_PACKAGE_PATTERN_FILE << std::endl;
    std::ifstream fin(config_path + LOG_CONFIGURATION_PACKAGE_PATTERN_FILE);
    std::getline(fin, pattern);

    // log path
    CheckPath(STRATEGY_PACKAGE_LOG_FOLDER);
    string log_path{STRATEGY_PACKAGE_LOG_FOLDER + log_file_name};
    if (log_file_name.find('.') == string::npos)
        log_path += ".log";
    // file appender
    SharedAppenderPtr fileAppender(new RollingFileAppender(log_path, STRATEGY_PACKAGE_LOG_MAX_FILE_SIZE, STRATEGY_PACKAGE_LOG_MAX_BACKUP_INDEX));
    fileAppender->setLayout(std::unique_ptr<Layout>(new PatternLayout(pattern)));

    // get a instance of log4cplus
    logger = Logger::getInstance(log_unique_name);

    logger.addAppender(fileAppender);
    logger.setLogLevel(DEBUG_LOG_LEVEL);
}