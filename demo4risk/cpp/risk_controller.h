#pragma once

#include "pandora/messager/ut_log.h"
#include "pandora/redis/redis_api.h"
#include "pandora/util/json.hpp"

#include "risk_controller_define.h"
#include "quote_updater.h"
#include "config_updater.h"
#include "account_updater.h"
#include "datacenter.h"

class RiskController : public IAccountUpdater, public IQuoteUpdater, public IConfigurationUpdater
{
public:
    RiskController();
    ~RiskController();

    void start();

    // signal handler function
    static volatile int signal_sys;
    static void signal_handler(int signum);


    // callback
    void on_snap(const QuoteData& quote);
    void on_configuration_update(const QuoteConfiguration& config);
    void on_account_update(const AccountInfo& info);
private:

    ConfigurationUpdater configuration_updater_;

    AccountUpdater account_updater_;

    QuoteUpdater quote_updater_;

    DataCenter datacenter_;
};