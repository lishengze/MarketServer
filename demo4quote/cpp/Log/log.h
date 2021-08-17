#include "../global_declare.h"

#include "pandora/util/thread_safe_singleton.hpp"
#include "pandora/log/log.h"

class SDepthQuote;
class Trade;
class KlineData;
class QuoteLog:public Log 
{
public:

    ~QuoteLog();

    QuoteLog();

    void record_input_info(const string& info, const SDepthQuote& quote);

    void record_input_info(const string& info, const Trade& trade);

    void record_input_info(const string& info, const vector<KlineData>& klines);

    void record_output_info(const string& info, const SDepthQuote& quote);

    void record_output_info(const string& info, const Trade& trade);

    void record_output_info(const string& info, const vector<KlineData>& klines);
};

#define LOG utrade::pandora::ThreadSafeSingleton<QuoteLog>::DoubleCheckInstance()

#define LOG_TRACE(info) LOG->log_trace(LOG_HEADER + info)
#define LOG_DEBUG(info) LOG->log_debug(LOG_HEADER + info)
#define LOG_INFO(info) LOG->log_info(LOG_HEADER + info)
#define LOG_WARN(info) LOG->log_warn(LOG_HEADER + info)
#define LOG_ERROR(info) LOG->log_error(LOG_HEADER + info)
#define LOG_FATAL(info) LOG->log_fatal(LOG_HEADER + info)

#define LOG_CLIENT_REQUEST(info) LOG->log_client_request(LOG_HEADER + info)
#define LOG_CLIENT_RESPONSE(info) LOG->log_client_response(LOG_HEADER + info)
#define LOG_SOURCE_INPUT(info) LOG->log_source_input(LOG_HEADER + info)
