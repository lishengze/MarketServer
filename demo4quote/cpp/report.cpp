#include "report.h"
#include "lib/reporter.h"
#include "stream_engine_config.h"
#include "quote_mixer2.h"

using namespace CProjUtil;

void Report::start()
{
    thread_run_ = true;
    loop_ = new std::thread(&Report::_looping, this);
}

void Report::_looping()
{    
    type_tick start_time = get_miliseconds();

    _log_and_print("[Monitor] connect to %s:%s.", CONFIG->reporter_addr_, CONFIG->reporter_port_);
    ReporterInterface::StartMonitor(CONFIG->reporter_addr_.c_str(), CONFIG->reporter_port_.c_str());

    char m_local_ip[32];
    int _localHostSize = 32;
    char _localHost[32];
    while( thread_run_ )
    {
        if( ReporterInterface::GetLocalHost(_localHost, _localHostSize) && _localHostSize > 0 && _localHostSize < 30)
        {
            _localHost[_localHostSize] = '\0';
            strcpy(m_local_ip, _localHost);
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    _log_and_print("[Monitor] local ip is %s.", _localHost);

    while( thread_run_ ) 
    {
        string typeName = "bcts";
        string aliasName = "quote";
        ReporterMsg* _msg = ReporterInterface::CreateMontiorMsg(_localHost, aliasName.c_str(), typeName.c_str());
        if( _msg )
        {
            cout << CONFIG->GLOBAL_HUOBI_BTC << ", " << CONFIG->GLOBAL_BINANCE_BTC << endl;
            ReporterInterface::AddField(_msg, "2", ToString(start_time/1000).c_str());
            ReporterInterface::AddField(_msg, "3", ToString(get_miliseconds()/1000).c_str());
            ReporterInterface::AddField(_msg, "1", PROG_VERSION);
            ReporterInterface::AddField(_msg, "4", ToString(CONFIG->GLOBAL_HUOBI_BTC).c_str()); // huobi.btc
            ReporterInterface::AddField(_msg, "5", ToString(CONFIG->GLOBAL_BINANCE_BTC).c_str()); // binance.btc
            ReporterInterface::AddField(_msg, "6", ToString(CONFIG->GLOBAL_OKEX_BTC).c_str()); // okex.btc
            ReporterInterface::AddField(_msg, "7", ToString(CONFIG->GLOBAL_BCTS_BTC).c_str()); // bcts.btc

            ReporterInterface::SendMsg(_msg);
        }
        // 休眠
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}